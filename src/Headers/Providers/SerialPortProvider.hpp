#pragma once

class SerialPortProvider : public ILogProvider
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

		switch (InLogLevel)
		{
			case ELogLevel::Trace:
				SerialWrite("   TRACE   :  ", 14);
				break;
			
			case ELogLevel::Debug:
				SerialWrite("   DEBUG   :  ", 14);
				break;
			
			case ELogLevel::Information:
				SerialWrite("    INF    :  ", 14);
				break;
			
			case ELogLevel::Warning:
				SerialWrite("    WRN    :  ", 14);
				break;
			
			case ELogLevel::Error:
				SerialWrite("   ERROR   :  ", 14);
				break;
			
			case ELogLevel::Fatal:
				SerialWrite("   FATAL   :  ", 14);
				break;

			default:
                SerialWrite("    UNK    :  ", 14);
				break;
		}

        // 
		// Convert the UNICODE string to an ANSI string.
		// 

		UNICODE_STRING UnicodeMessage;
		UnicodeMessage.Buffer = (PWCH) InMessage;
		UnicodeMessage.Length = (USHORT) wcslen(InMessage) * sizeof(WCHAR);
		UnicodeMessage.MaximumLength = UnicodeMessage.Length + sizeof(WCHAR);
		
		ANSI_STRING AnsiMessage;
		AnsiMessage.Buffer = (PCHAR) ExAllocatePoolZero(NonPagedPoolNx, UnicodeMessage.MaximumLength, LOGGER_NT_POOL_TAG);
		AnsiMessage.Length = 0;
		AnsiMessage.MaximumLength = UnicodeMessage.MaximumLength;
		
		if (NT_SUCCESS(RtlUnicodeStringToAnsiString(&AnsiMessage, &UnicodeMessage, false)))
		{
			//
			// Write the message.
			// 

			SerialWrite(AnsiMessage.Buffer, AnsiMessage.Length);
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

private:

	static void SerialWrite(CONST CHAR* InBuffer, SIZE_T InLength)
	{
		WRITE_PORT_BUFFER_UCHAR((PUCHAR) 0x3F8, (PUCHAR) InBuffer, (ULONG) InLength);
    }

	static void SerialWrite(CONST WCHAR* InBuffer, SIZE_T InLength)
	{
		for (SIZE_T Idx = 0; Idx < InLength; ++Idx)
		    WRITE_PORT_UCHAR((PUCHAR) 0x3F8, (InBuffer[Idx] < 0x80) ? (UCHAR) InBuffer[Idx] : '?');
    }
};