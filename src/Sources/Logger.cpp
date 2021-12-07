#include "../Headers/LoggerNT.h"
using namespace LoggerNT;

/// <summary>
/// Initializes the LoggerNT library.
/// </summary>
/// <param name="InConfig">The configuration.</param>
NTSTATUS LogInitLibrary(CONST LoggerConfig& InConfig)
{
	if (IsSetup == FALSE)
	{
		KeInitializeSpinLock(&ProvidersLock);
		KeInitializeSpinLock(&LogProcessingLock);
		IsSetup = TRUE;
	}

	Config = InConfig;
	return STATUS_SUCCESS;
}

/// <summary>
/// Logs a message of the specified log level.
/// </summary>
/// <param name="InLogLevel">The severity.</param>
/// <param name="InFormat">The format of the message.</param>
/// <param name="InArguments">The arguments for the message format.</param>
void Logv(ELogLevel InLogLevel, CONST WCHAR* InFormat, va_list InArguments)
{
	// 
	// Check whether this log should be processed or not.
	// 

	if (InLogLevel < Config.MinimumLevel)
		return;
	
	// 
	// Calculate the number of bytes required for the formatting.
	// 

	auto const NumberOfCharactersRequired = _vsnwprintf(nullptr, 0, InFormat, InArguments);

	if (NumberOfCharactersRequired < 0)
		return;

	// 
	// Lock the logger, and check if we already have allocated enough memory for the formatting.
	// 
	
	KIRQL OldIrql;
	KeAcquireSpinLock(&LogProcessingLock, &OldIrql);

	if (LogProcessingBuffer == nullptr ||
		LogProcessBufferSize < (NumberOfCharactersRequired + 2) * sizeof(WCHAR))
	{
		if (LogProcessingBuffer != nullptr)
			ExFreePoolWithTag(LogProcessingBuffer, 'Log ');
		
		LogProcessBufferSize = (NumberOfCharactersRequired + 2) * sizeof(WCHAR);
		LogProcessingBuffer = (WCHAR*) ExAllocatePoolZero(NonPagedPoolNx, LogProcessBufferSize, 'Log ');

		if (LogProcessingBuffer == nullptr)
		{
			// 
			// We don't have enough memory on the system.
			// 

			LogProcessBufferSize = 0;
			LogProcessingBuffer = nullptr;
			KeReleaseSpinLock(&LogProcessingLock, OldIrql);
			return;
		}
	}

	// 
	// Format the message and the arguments.
	// 

	if (vswprintf_s(LogProcessingBuffer, LogProcessBufferSize / sizeof(*InFormat), InFormat, InArguments) < 0)
	{
		KeReleaseSpinLock(&LogProcessingLock, OldIrql);
		return;
	}

	// 
	// Append a break-line at the end of the message.
	// 

	LogProcessingBuffer[NumberOfCharactersRequired] = L'\n';
	LogProcessingBuffer[NumberOfCharactersRequired + 1] = L'\0';
	
	// 
	// Log the message.
	// 
	
	KeAcquireSpinLockAtDpcLevel(&ProvidersLock);

	for (LONG ProviderIdx = 0; ProviderIdx < NumberOfProviders; ++ProviderIdx)
	{
		if (auto* Provider = Providers[ProviderIdx]; Provider != nullptr)
			Provider->Log(InLogLevel, LogProcessingBuffer);
	}

	KeReleaseSpinLockFromDpcLevel(&ProvidersLock),
	KeReleaseSpinLock(&LogProcessingLock, OldIrql);
}

/// <summary>
/// Logs a message of the specified log level.
/// </summary>
/// <param name="InLogLevel">The severity.</param>
/// <param name="InFormat">The format of the message.</param>
/// <param name="...">The arguments for the message format.</param>
void Log(ELogLevel InLogLevel, CONST WCHAR* InFormat, ...)
{
	va_list Arguments;
	va_start(Arguments, InFormat);
	Logv(InLogLevel, InFormat, Arguments);
}

/// <summary>
/// Logs a message with the 'Trace' severity level.
/// </summary>
/// <param name="InFormat">The format of the message.</param>
/// <param name="...">The arguments for the message format.</param>
void LogTrace(CONST WCHAR* InFormat, ...)
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
void LogDebug(CONST WCHAR* InFormat, ...)
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
void LogInfo(CONST WCHAR* InFormat, ...)
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
void LogWarning(CONST WCHAR* InFormat, ...)
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
void LogError(CONST WCHAR* InFormat, ...)
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
void LogFatal(CONST WCHAR* InFormat, ...)
{
	va_list Arguments;
	va_start(Arguments, InFormat);
	Logv(ELogLevel::Fatal, InFormat, Arguments);
}
