#pragma once

inline void* __cdecl operator new(size_t size, void* location)
{
	UNREFERENCED_PARAMETER(size);
	return location;
}

namespace LoggerNT
{
	/// <summary>
	/// The configuration of the logging system.
	/// </summary>
	inline LoggerConfig Config = { };

	/// <summary>
	/// Whether this class has been setup or not.
	/// </summary>
	inline BOOLEAN IsSetup = FALSE;

	/// <summary>
	/// The synchronization spin lock for the providers list.
	/// </summary>
	inline KSPIN_LOCK ProvidersLock = { };

	/// <summary>
	/// The list of providers currently used by this logger.
	/// </summary>
	inline ILogProvider* Providers[16] = { };

	/// <summary>
	/// The number of entries in the list of providers.
	/// </summary>
	inline LONG NumberOfProviders = 0;

	/// <summary>
	/// The synchronization spin lock for log processing.
	/// </summary>
	inline KSPIN_LOCK LogProcessingLock = { };

	/// <summary>
	/// The buffer used to store the formatted output.
	/// </summary>
	inline WCHAR* LogProcessingBuffer = nullptr;

	/// <summary>
	/// The size of the buffer used to store the formatted output.
	/// </summary>
	inline SIZE_T LogProcessBufferSize = 0;
}

/// <summary>
/// Initializes the LoggerNT library.
/// </summary>
/// <param name="InConfig">The configuration.</param>
NTSTATUS LogInitLibrary(CONST LoggerConfig& InConfig);

/// <summary>
/// Adds a logging provider to this logger instance.
/// </summary>
/// <param name="InProvider">An existing instance of the logging provider.</param>
template <class TProvider>
TProvider* LogAddProvider(OPTIONAL TProvider* InProvider = nullptr)
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
	KeAcquireSpinLock(&LoggerNT::ProvidersLock, &OldIrql);
	InterlockedExchangePointer((PVOID*) &LoggerNT::Providers[InterlockedIncrement(&LoggerNT::NumberOfProviders) - 1], InProvider);
	KeReleaseSpinLock(&LoggerNT::ProvidersLock, OldIrql);
	return InProvider;
}

/// <summary>
/// Logs a message of the specified log level.
/// </summary>
/// <param name="InLogLevel">The severity.</param>
/// <param name="InFormat">The format of the message.</param>
/// <param name="InArguments">The arguments for the message format.</param>
void Logv(ELogLevel InLogLevel, CONST WCHAR* InFormat, va_list InArguments);

/// <summary>
/// Logs a message of the specified log level.
/// </summary>
/// <param name="InLogLevel">The severity.</param>
/// <param name="InFormat">The format of the message.</param>
/// <param name="...">The arguments for the message format.</param>
void Log(ELogLevel InLogLevel, CONST WCHAR* InFormat, ...);

/// <summary>
/// Logs a message with the 'Trace' severity level.
/// </summary>
/// <param name="InFormat">The format of the message.</param>
/// <param name="...">The arguments for the message format.</param>
void LogTrace(CONST WCHAR* InFormat, ...);

/// <summary>
/// Logs a message with the 'Debug' severity level.
/// </summary>
/// <param name="InFormat">The format of the message.</param>
/// <param name="...">The arguments for the message format.</param>
void LogDebug(CONST WCHAR* InFormat, ...);

/// <summary>
/// Logs a message with the 'Information' severity level.
/// </summary>
/// <param name="InFormat">The format of the message.</param>
/// <param name="...">The arguments for the message format.</param>
void LogInfo(CONST WCHAR* InFormat, ...);

/// <summary>
/// Logs a message with the 'Warning' severity level.
/// </summary>
/// <param name="InFormat">The format of the message.</param>
/// <param name="...">The arguments for the message format.</param>
void LogWarning(CONST WCHAR* InFormat, ...);

/// <summary>
/// Logs a message with the 'Error' severity level.
/// </summary>
/// <param name="InFormat">The format of the message.</param>
/// <param name="...">The arguments for the message format.</param>
void LogError(CONST WCHAR* InFormat, ...);

/// <summary>
/// Logs a message with the 'Fatal' severity level.
/// </summary>
/// <param name="InFormat">The format of the message.</param>
/// <param name="...">The arguments for the message format.</param>
void LogFatal(CONST WCHAR* InFormat, ...);
