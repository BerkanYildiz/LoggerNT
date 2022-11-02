#pragma once

class DbgPrintProvider : public ILogProvider
{
public:
	
	/// <summary>
	/// Logs a message of the specified severity.
	/// </summary>
	/// <param name="InLogLevel">The severity.</param>
	/// <param name="InMessage">The message.</param>
	void Log(ELogLevel InLogLevel, CONST WCHAR* InMessage) override
	{
		// 
		// Select the correct prefix for this log level.
		// 
		
		ANSI_STRING LevelPrefix;

		switch (InLogLevel)
		{
			case ELogLevel::Trace:
				RtlInitAnsiString(&LevelPrefix, " TRACE : ");
				break;
			
			case ELogLevel::Debug:
				RtlInitAnsiString(&LevelPrefix, " DEBUG : ");
				break;
			
			case ELogLevel::Information:
				RtlInitAnsiString(&LevelPrefix, "  INF  : ");
				break;
			
			case ELogLevel::Warning:
				RtlInitAnsiString(&LevelPrefix, "  WRN  : ");
				break;
			
			case ELogLevel::Error:
				RtlInitAnsiString(&LevelPrefix, " ERROR : ");
				break;
			
			case ELogLevel::Fatal:
				RtlInitAnsiString(&LevelPrefix, " FATAL : ");
				break;

			default:
				RtlInitAnsiString(&LevelPrefix, "  UNK  : ");
				break;
		}
		
		// 
		// Convert the UNICODE string to a ANSI string.
		// 

		UNICODE_STRING UnicodeMessage;
		UnicodeMessage.Buffer = (PWCH) InMessage;
		UnicodeMessage.Length = (USHORT) wcslen(InMessage) * 2;
		UnicodeMessage.MaximumLength = UnicodeMessage.Length + 1;
		
		ANSI_STRING AnsiMessage;
		AnsiMessage.Buffer = (PCHAR) ExAllocatePoolZero(NonPagedPoolNx, UnicodeMessage.MaximumLength + LevelPrefix.Length, LOGGER_NT_POOL_TAG);

		if (AnsiMessage.Buffer == nullptr)
			return;
		
		AnsiMessage.Length = 0;
		AnsiMessage.MaximumLength = UnicodeMessage.MaximumLength + LevelPrefix.Length;

		// 
		// Apply the conversion but leave some space at the beginning for the level prefix.
		// 

		ANSI_STRING AnsiMessageForConversion;
		AnsiMessageForConversion.Buffer = (CHAR*) ((UINT64) AnsiMessage.Buffer + LevelPrefix.Length);
		AnsiMessageForConversion.Length = 0;
		AnsiMessageForConversion.MaximumLength = UnicodeMessage.MaximumLength;
		
		if (!NT_SUCCESS(RtlUnicodeStringToAnsiString(&AnsiMessageForConversion, &UnicodeMessage, false)))
		{
			ExFreePoolWithTag(AnsiMessage.Buffer, LOGGER_NT_POOL_TAG);
			return;
		}

		// 
		// Copy the prefix at the beginning of the buffer.
		// 

		AnsiMessage.Length = AnsiMessageForConversion.Length + LevelPrefix.Length;
		RtlCopyMemory(AnsiMessage.Buffer, LevelPrefix.Buffer, LevelPrefix.Length);

		// 
		// Log the message.
		// 

		switch (InLogLevel)
		{
			case ELogLevel::Trace:
			case ELogLevel::Debug:
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, AnsiMessage.Buffer);
				break;

			case ELogLevel::Information:
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, AnsiMessage.Buffer);
				break;

			case ELogLevel::Warning:
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, AnsiMessage.Buffer);
				break;

			case ELogLevel::Error:
			case ELogLevel::Fatal:
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, AnsiMessage.Buffer);
				break;
		}

		// 
		// Release the memory allocated for the conversion.
		// 

		ExFreePoolWithTag(AnsiMessage.Buffer, LOGGER_NT_POOL_TAG);
	}

	/// <summary>
	/// Destroys this log provider.
	/// </summary>
	void Exit() override
	{
		// ...
	}
};