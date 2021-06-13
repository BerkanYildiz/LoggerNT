#include "../Headers/LoggerNtLib.hpp"

/// <summary>
/// Logs a message of the specified severity.
/// </summary>
/// <param name="InLogLevel">The severity.</param>
/// <param name="InFormat">The format of the message.</param>
/// <param name="...">The arguments for the message format.</param>
void Logger::Log(ELogLevel InLogLevel, const TCHAR* InFormat, ...)
{
	// 
	// Check whether this log should be processed or not.
	// 

	if (InLogLevel < this->Config.MinimumLevel)
	{
		
	}
	
	// 
	// Calculate the number of bytes required for the formatting.
	// 

	va_list Arguments;
	__va_start(&Arguments, InFormat);
	auto const NumberOfCharactersRequired = _vscwprintf(InFormat, Arguments);

	if (NumberOfCharactersRequired < 0)
	{
		_crt_va_end(Arguments);
		return;
	}

	// 
	// Lock the logger, and check if we already have allocated enough memory for the formatting.
	// 
	
	KIRQL OldIrql;
	KeAcquireSpinLock(&LogProcessingLock, &OldIrql);

	if (this->LogProcessingBuffer == nullptr ||
		this->LogProcessBufferSize < (NumberOfCharactersRequired + 1) * sizeof(TCHAR))
	{
		if (this->LogProcessingBuffer != nullptr)
			ExFreePoolWithTag(this->LogProcessingBuffer, 'Log ');
		
		this->LogProcessBufferSize = (NumberOfCharactersRequired + 1) * sizeof(TCHAR);
		this->LogProcessingBuffer = (TCHAR*) ExAllocatePoolZero(NonPagedPoolNx, this->LogProcessBufferSize, 'Log ');

		if (this->LogProcessingBuffer == nullptr)
		{
			// 
			// We don't have enough memory on the system.
			// 

			this->LogProcessBufferSize = 0;
			this->LogProcessingBuffer = nullptr;
			KeReleaseSpinLock(&LogProcessingLock, OldIrql);
			_crt_va_end(Arguments);
			return;
		}
	}

	// 
	// Format the message and the arguments.
	// 

	if (vswprintf_s(this->LogProcessingBuffer, this->LogProcessBufferSize / sizeof(TCHAR), InFormat, Arguments) < 0)
	{
		KeReleaseSpinLock(&LogProcessingLock, OldIrql);
		_crt_va_end(Arguments);
		return;
	}

	_crt_va_end(Arguments);

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
