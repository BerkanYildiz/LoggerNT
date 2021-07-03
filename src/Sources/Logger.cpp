#include "../Headers/LoggerNT.h"

/// <summary>
/// Logs a message of the specified severity.
/// </summary>
/// <param name="InLogLevel">The severity.</param>
/// <param name="InFormat">The format of the message.</param>
/// <param name="...">The arguments for the message format.</param>
void Logger::Log(ELogLevel InLogLevel, const WCHAR* InFormat, ...)
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
void Logger::Logv(ELogLevel InLogLevel, const WCHAR* InFormat, va_list InArguments)
{
	// 
	// Check whether this log should be processed or not.
	// 

	if (InLogLevel < this->Config.MinimumLevel)
		return;
	
	// 
	// Calculate the number of bytes required for the formatting.
	// 
	
	auto const NumberOfCharactersRequired = _vscwprintf(InFormat, InArguments);

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

void Tests_Example()
{
	auto Logger = ::Logger();

	// Print with DbgPrintEx
	Logger.AddProvider<DbgPrintProvider>();

	// Write to a file
	auto FileProvider = ::TempFileProvider(L"debug.log");
	FileProvider.UseFileNamed(L"debug.log"); // or you can call a function instead of constructor
	Logger.AddProvider<TempFileProvider>(&FileProvider);

	// Log something explicitly
	Logger.Log(ELogLevel::Debug, L"Example message one");

	// Log using a level function
	Logger.LogDebug(L"Example message one");

	// Log with an error level
	Logger.Log(ELogLevel::Error, L"Example message two");
	Logger.LogError(L"Example message two");
}