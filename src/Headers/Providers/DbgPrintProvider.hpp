#pragma once

class DbgPrintProvider : public ILogProvider
{
public:
	
	/// <summary>
	/// Logs a message of the specified severity.
	/// </summary>
	/// <param name="InLogLevel">The severity.</param>
	/// <param name="InMessage">The message.</param>
	void Log(ELogLevel InLogLevel, const WCHAR* InMessage) override
	{
		// 
		// Convert the UNICODE string to a ANSI string.
		// 

		UNICODE_STRING UnicodeMessage;
		UnicodeMessage.Buffer = (PWCH) InMessage;
		UnicodeMessage.Length = (USHORT) wcslen(InMessage) * 2;
		UnicodeMessage.MaximumLength = UnicodeMessage.Length + 1;
		
		ANSI_STRING AnsiMessage;
		AnsiMessage.Buffer = (PCHAR) ExAllocatePoolZero(NonPagedPoolNx, UnicodeMessage.MaximumLength, 'Log ');

		if (AnsiMessage.Buffer == nullptr)
			return;
		
		AnsiMessage.Length = 0;
		AnsiMessage.MaximumLength = UnicodeMessage.MaximumLength;
		
		if (!NT_SUCCESS(RtlUnicodeStringToAnsiString(&AnsiMessage, &UnicodeMessage, false)))
		{
			ExFreePoolWithTag(AnsiMessage.Buffer, 'Log ');
			return;
		}

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

		ExFreePoolWithTag(AnsiMessage.Buffer, 'Log ');
	}
};