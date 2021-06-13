#pragma once

class DbgPrintProvider : public ILogProvider
{
public:
	
	/// <summary>
	/// Logs a message of the specified severity.
	/// </summary>
	/// <param name="InLogLevel">The severity.</param>
	/// <param name="InMessage">The message.</param>
	void Log(ELogLevel InLogLevel, const TCHAR* InMessage) override
	{
		// 
		// Convert the UNICODE string to a ANSI string.
		// 

		UNICODE_STRING UnicodeMessage;
		UnicodeMessage.Buffer = (PWCH) InMessage;
		UnicodeMessage.Length = (USHORT) wcslen(InMessage);
		UnicodeMessage.MaximumLength = UnicodeMessage.Length;
		
		ANSI_STRING AnsiMessage;
		AnsiMessage.Buffer = (PCHAR) ExAllocatePoolZero(NonPagedPoolNx, UnicodeMessage.MaximumLength, 'Log ');
		AnsiMessage.Length = 0;
		AnsiMessage.MaximumLength = UnicodeMessage.MaximumLength;
		
		if (!NT_SUCCESS(RtlUnicodeStringToAnsiString(&AnsiMessage, &UnicodeMessage, false)))
			return;

		// 
		// Log the message.
		// 
		
		switch (InLogLevel)
		{
			case ELogLevel::Trace:
			case ELogLevel::Debug:
				DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, AnsiMessage.Buffer);
				break;

			case ELogLevel::Information:
				DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL, AnsiMessage.Buffer);
				break;

			case ELogLevel::Warning:
				DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_WARNING_LEVEL, AnsiMessage.Buffer);
				break;

			case ELogLevel::Error:
			case ELogLevel::Fatal:
				DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, AnsiMessage.Buffer);
				break;
		}

		// 
		// Release the memory allocated for the conversion.
		// 

		ExFreePoolWithTag(AnsiMessage.Buffer, 'Log ');
	}
};