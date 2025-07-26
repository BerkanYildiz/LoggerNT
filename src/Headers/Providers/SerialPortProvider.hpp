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
				SerialWrite(L"   TRACE   :  ", 14);
				break;
			
			case ELogLevel::Debug:
				SerialWrite(L"   DEBUG   :  ", 14);
				break;
			
			case ELogLevel::Information:
				SerialWrite(L"    INF    :  ", 14);
				break;
			
			case ELogLevel::Warning:
				SerialWrite(L"    WRN    :  ", 14);
				break;
			
			case ELogLevel::Error:
				SerialWrite(L"   ERROR   :  ", 14);
				break;
			
			case ELogLevel::Fatal:
				SerialWrite(L"   FATAL   :  ", 14);
				break;

			default:
                SerialWrite(L"    UNK    :  ", 14);
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
		WRITE_PORT_BUFFER_UCHAR((PUCHAR) 0x3F8, (PUCHAR) InBuffer, InLength * 2);
    }
};