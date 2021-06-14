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
	TCHAR* LogProcessingBuffer = nullptr;

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
	void Log(ELogLevel InLogLevel, const TCHAR* InFormat, ...);
	
	/// <summary>
	/// Logs a message of the specified log level.
	/// </summary>
	/// <param name="InLogLevel">The severity.</param>
	/// <param name="InFormat">The format of the message.</param>
	/// <param name="InArguments">The arguments for the message format.</param>
	void Logv(ELogLevel InLogLevel, const TCHAR* InFormat, va_list InArguments);

public:

	/// <summary>
	/// Logs a message with the 'Trace' severity level.
	/// </summary>
	/// <param name="InFormat">The format of the message.</param>
	/// <param name="...">The arguments for the message format.</param>
	void LogTrace(const TCHAR* InFormat, ...)
	{
		va_list Arguments;
		__va_start(&Arguments, InFormat);
		Logv(ELogLevel::Trace, InFormat, Arguments);
	}

	/// <summary>
	/// Logs a message with the 'Debug' severity level.
	/// </summary>
	/// <param name="InFormat">The format of the message.</param>
	/// <param name="...">The arguments for the message format.</param>
	void LogDebug(const TCHAR* InFormat, ...)
	{
		va_list Arguments;
		__va_start(&Arguments, InFormat);
		Logv(ELogLevel::Debug, InFormat, Arguments);
	}

	/// <summary>
	/// Logs a message with the 'Information' severity level.
	/// </summary>
	/// <param name="InFormat">The format of the message.</param>
	/// <param name="...">The arguments for the message format.</param>
	void LogInformation(const TCHAR* InFormat, ...)
	{
		va_list Arguments;
		__va_start(&Arguments, InFormat);
		Logv(ELogLevel::Information, InFormat, Arguments);
	}

	/// <summary>
	/// Logs a message with the 'Warning' severity level.
	/// </summary>
	/// <param name="InFormat">The format of the message.</param>
	/// <param name="...">The arguments for the message format.</param>
	void LogWarning(const TCHAR* InFormat, ...)
	{
		va_list Arguments;
		__va_start(&Arguments, InFormat);
		Logv(ELogLevel::Warning, InFormat, Arguments);
	}

	/// <summary>
	/// Logs a message with the 'Error' severity level.
	/// </summary>
	/// <param name="InFormat">The format of the message.</param>
	/// <param name="...">The arguments for the message format.</param>
	void LogError(const TCHAR* InFormat, ...)
	{
		va_list Arguments;
		__va_start(&Arguments, InFormat);
		Logv(ELogLevel::Error, InFormat, Arguments);
	}

	/// <summary>
	/// Logs a message with the 'Fatal' severity level.
	/// </summary>
	/// <param name="InFormat">The format of the message.</param>
	/// <param name="...">The arguments for the message format.</param>
	void LogFatal(const TCHAR* InFormat, ...)
	{
		va_list Arguments;
		__va_start(&Arguments, InFormat);
		Logv(ELogLevel::Fatal, InFormat, Arguments);
	}
};
