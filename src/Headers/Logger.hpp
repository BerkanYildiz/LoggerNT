#pragma once

inline void* __cdecl operator new(size_t size, void* location)
{
	UNREFERENCED_PARAMETER(size);
	return location;
}

class Logger
{
private:

	/// <summary>
	/// The configuration of the logging system.
	/// </summary>
	LoggerConfig Config = { };

private:

	/// <summary>
	/// The synchronization spin lock for the providers list.
	/// </summary>
	KSPIN_LOCK ProvidersLock = { };

	/// <summary>
	/// The list of providers currently used by this logger.
	/// </summary>
	ILogProvider* Providers[16] = { };

	/// <summary>
	/// The number of entries in the list of providers.
	/// </summary>
	LONG NumberOfProviders = 0;

private:

	/// <summary>
	/// The synchronization spin lock for log processing.
	/// </summary>
	KSPIN_LOCK LogProcessingLock = { };

	/// <summary>
	/// The buffer used to store the formatted output.
	/// </summary>
	WCHAR* LogProcessingBuffer = nullptr;

	/// <summary>
	/// The size of the buffer used to store the formatted output.
	/// </summary>
	SIZE_T LogProcessBufferSize = 0;

public:

	/// <summary>
	/// Initializes a new instance of the <see cref="Logger"/> class.
	/// </summary>
	Logger()
	{
		KeInitializeSpinLock(&ProvidersLock);
		KeInitializeSpinLock(&LogProcessingLock);
	}

	/// <summary>
	/// Initializes a new instance of the <see cref="Logger"/> class.
	/// </summary>
	/// <param name="InConfig">The configuration.</param>
	Logger(const LoggerConfig& InConfig) : Logger()
	{
		this->Config = InConfig;
	}

public:
	
	/// <summary>
	/// Adds a logging provider to this logger instance.
	/// </summary>
	/// <param name="InProvider">An existing instance of the logging provider.</param>
	template <class TProvider>
	bool AddProvider(OPTIONAL TProvider* InProvider = nullptr)
	{
		static_assert(__is_base_of(::ILogProvider, TProvider), "The logging provider is not based on ILogProvider");
		
		// 
		// If a provider instance wasn't specified, create one.
		// 

		if (InProvider == nullptr)
		{
			InProvider = (TProvider*) ExAllocatePoolZero(NonPagedPoolNx, sizeof(TProvider), 'Log ');
			InProvider = new(InProvider) TProvider();
		}
		
		// 
		// Add the provider to the list of providers.
		// 

		KIRQL OldIrql;
		KeAcquireSpinLock(&ProvidersLock, &OldIrql);
		InterlockedExchangePointer((PVOID*) &Providers[InterlockedIncrement(&NumberOfProviders) - 1], InProvider);
		KeReleaseSpinLock(&ProvidersLock, OldIrql);
		return true;
	}
	
	/// <summary>
	/// Logs a message of the specified log level.
	/// </summary>
	/// <param name="InLogLevel">The severity.</param>
	/// <param name="InFormat">The format of the message.</param>
	/// <param name="...">The arguments for the message format.</param>
	void Log(ELogLevel InLogLevel, const WCHAR* InFormat, ...)
	{
		va_list Arguments;
		va_start(Arguments, InFormat);
		Logv(InLogLevel, InFormat, Arguments);
	}
	
	/// <summary>
	/// Logs a message of the specified log level.
	/// </summary>
	/// <param name="InLogLevel">The severity.</param>
	/// <param name="InFormat">The format of the message.</param>
	/// <param name="InArguments">The arguments for the message format.</param>
	void Logv(ELogLevel InLogLevel, const WCHAR* InFormat, va_list InArguments)
	{
		// 
		// Check whether this log should be processed or not.
		// 

		if (InLogLevel < this->Config.MinimumLevel)
			return;
		
		// 
		// Calculate the number of bytes required for the formatting.
		// 
		
		// auto const NumberOfCharactersRequired = _vscwprintf(InFormat, InArguments);
		auto const NumberOfCharactersRequired = _vsnwprintf(nullptr, 0, InFormat, InArguments);

		if (NumberOfCharactersRequired < 0)
			return;

		// 
		// Lock the logger, and check if we already have allocated enough memory for the formatting.
		// 
		
		KIRQL OldIrql;
		KeAcquireSpinLock(&LogProcessingLock, &OldIrql);

		if (this->LogProcessingBuffer == nullptr ||
			this->LogProcessBufferSize < (NumberOfCharactersRequired + 1) * sizeof(WCHAR))
		{
			if (this->LogProcessingBuffer != nullptr)
				ExFreePoolWithTag(this->LogProcessingBuffer, 'Log ');
			
			this->LogProcessBufferSize = (NumberOfCharactersRequired + 1) * sizeof(WCHAR);
			this->LogProcessingBuffer = (WCHAR*) ExAllocatePoolZero(NonPagedPoolNx, this->LogProcessBufferSize, 'Log ');

			if (this->LogProcessingBuffer == nullptr)
			{
				// 
				// We don't have enough memory on the system.
				// 

				this->LogProcessBufferSize = 0;
				this->LogProcessingBuffer = nullptr;
				KeReleaseSpinLock(&LogProcessingLock, OldIrql);
				return;
			}
		}

		// 
		// Format the message and the arguments.
		// 

		if (vswprintf_s(this->LogProcessingBuffer, this->LogProcessBufferSize / sizeof(WCHAR), InFormat, InArguments) < 0)
		{
			KeReleaseSpinLock(&LogProcessingLock, OldIrql);
			return;
		}

		// 
		// Log the message.
		// 

		KIRQL OldIrqlBis;
		KeAcquireSpinLock(&ProvidersLock, &OldIrqlBis);

		for (LONG ProviderIdx = 0; ProviderIdx < NumberOfProviders; ++ProviderIdx)
		{
			if (auto* Provider = Providers[ProviderIdx]; Provider != nullptr)
				Provider->Log(InLogLevel, this->LogProcessingBuffer);
		}

		KeReleaseSpinLock(&ProvidersLock, OldIrqlBis);
		KeReleaseSpinLock(&LogProcessingLock, OldIrql);
	}

public:

	/// <summary>
	/// Logs a message with the 'Trace' severity level.
	/// </summary>
	/// <param name="InFormat">The format of the message.</param>
	/// <param name="...">The arguments for the message format.</param>
	void LogTrace(const WCHAR* InFormat, ...)
	{
		va_list Arguments;
		va_start(Arguments, InFormat);
		Logv(ELogLevel::Trace, InFormat, Arguments);
	}

	/// <summary>
	/// Logs a message with the 'Debug' severity level.
	/// </summary>
	/// <param name="InFormat">The format of the message.</param>
	/// <param name="...">The arguments for the message format.</param>
	void LogDebug(const WCHAR* InFormat, ...)
	{
		va_list Arguments;
		va_start(Arguments, InFormat);
		Logv(ELogLevel::Debug, InFormat, Arguments);
	}

	/// <summary>
	/// Logs a message with the 'Information' severity level.
	/// </summary>
	/// <param name="InFormat">The format of the message.</param>
	/// <param name="...">The arguments for the message format.</param>
	void LogInformation(const WCHAR* InFormat, ...)
	{
		va_list Arguments;
		va_start(Arguments, InFormat);
		Logv(ELogLevel::Information, InFormat, Arguments);
	}

	/// <summary>
	/// Logs a message with the 'Warning' severity level.
	/// </summary>
	/// <param name="InFormat">The format of the message.</param>
	/// <param name="...">The arguments for the message format.</param>
	void LogWarning(const WCHAR* InFormat, ...)
	{
		va_list Arguments;
		va_start(Arguments, InFormat);
		Logv(ELogLevel::Warning, InFormat, Arguments);
	}

	/// <summary>
	/// Logs a message with the 'Error' severity level.
	/// </summary>
	/// <param name="InFormat">The format of the message.</param>
	/// <param name="...">The arguments for the message format.</param>
	void LogError(const WCHAR* InFormat, ...)
	{
		va_list Arguments;
		va_start(Arguments, InFormat);
		Logv(ELogLevel::Error, InFormat, Arguments);
	}

	/// <summary>
	/// Logs a message with the 'Fatal' severity level.
	/// </summary>
	/// <param name="InFormat">The format of the message.</param>
	/// <param name="...">The arguments for the message format.</param>
	void LogFatal(const WCHAR* InFormat, ...)
	{
		va_list Arguments;
		va_start(Arguments, InFormat);
		Logv(ELogLevel::Fatal, InFormat, Arguments);
	}
};
