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
				SerialWrite(L" TRACE : ", 9);
				break;
			
			case ELogLevel::Debug:
				SerialWrite(L" DEBUG : ", 9);
				break;
			
			case ELogLevel::Information:
				SerialWrite(L"  INF  : ", 9);
				break;
			
			case ELogLevel::Warning:
				SerialWrite(L"  WRN  : ", 9);
				break;
			
			case ELogLevel::Error:
				SerialWrite(L" ERROR : ", 9);
				break;
			
			case ELogLevel::Fatal:
				SerialWrite(L" FATAL : ", 9);
				break;

			default:
                SerialWrite(L"  UNK  : ", 9);
				break;
		}

		//
		// Write the message.
		// 

		SerialWrite(InMessage, wcslen(InMessage));
	}

	/// <summary>
	/// Destroys this log provider.
	/// </summary>
	void Exit() override
	{
		// ...
	}

private:

	static void SerialWrite(CONST WCHAR* InBuffer, ULONG InLength)
	{
		while (!(READ_PORT_UCHAR((PUCHAR) 0x3FD) & 0x20))
			_mm_pause();

		WRITE_PORT_BUFFER_USHORT((PUSHORT) 0x3F8, (PUSHORT) InBuffer, InLength);
    }
};