#pragma once

class TempFileProvider : public ILogProvider
{
private:
	
	/// <summary>
	/// The handle to the file saved on disk.
	/// </summary>
	HANDLE FileHandle = nullptr;

public:
	
	/// <summary>
	/// Whether the file should be in UNICODE or ANSI format.
	/// </summary>
	BOOLEAN ShouldStoreAsAnsi = FALSE;

public:
	
	/// <summary>
	/// Opens or creates a file with the specified name, in the temporary folder for system components.
	/// </summary>
	/// <param name="InFilename">The filename.</param>
	NTSTATUS UseFileNamed(CONST WCHAR* InFilename)
	{
		// 
		// If a file was was already open...
		// 

		if (this->FileHandle != nullptr)
		{
			ZwClose(this->FileHandle);
			this->FileHandle = nullptr;
		}
		
		// 
		// Build the path to the temporary system folder.
		// 

		WCHAR UnicodeFileNameBuffer[MAXIMUM_FILENAME_LENGTH] = { };
		UNICODE_STRING UnicodeFileName = { };
		RtlInitEmptyUnicodeString(&UnicodeFileName, UnicodeFileNameBuffer, sizeof(UnicodeFileNameBuffer));

		if (wcschr(InFilename, L'\\') == nullptr)
			RtlAppendUnicodeToString(&UnicodeFileName, L"\\SystemRoot\\Temp\\");
		
		RtlAppendUnicodeToString(&UnicodeFileName, InFilename);

		// 
		// Create or open the log file.
		// 

		NTSTATUS Status = { };
		IO_STATUS_BLOCK IoStatusBlock = { };

		OBJECT_ATTRIBUTES ObjectAttributes;
		InitializeObjectAttributes(&ObjectAttributes, &UnicodeFileName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

		if (!NT_SUCCESS(Status = ZwCreateFile(&FileHandle, GENERIC_WRITE | FILE_READ_ATTRIBUTES, &ObjectAttributes, &IoStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN_IF, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0)))
			return Status;

		// 
		// Seek to the end of the file.
		// 

		FILE_STANDARD_INFORMATION FileInformation = { };

		if (NT_SUCCESS(Status = ZwQueryInformationFile(FileHandle, &IoStatusBlock, &FileInformation, sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation)))
		{
			FILE_POSITION_INFORMATION FilePositionInformation = { };
			FilePositionInformation.CurrentByteOffset = FileInformation.EndOfFile;
			Status = ZwSetInformationFile(FileHandle, &IoStatusBlock, &FilePositionInformation, sizeof(FILE_POSITION_INFORMATION), ::FilePositionInformation);
		}

		return STATUS_SUCCESS;
	}

public:
	
	/// <summary>
	/// Logs a message of the specified severity.
	/// </summary>
	/// <param name="InLogLevel">The severity.</param>
	/// <param name="InMessage">The message.</param>
	void Log(ELogLevel InLogLevel, CONST WCHAR* InMessage) override
	{
		UNREFERENCED_PARAMETER(InLogLevel);
		
		// 
		// If no file was selected, we cannot do anything.
		// 

		if (this->FileHandle == nullptr)
			return;

		// 
		// Writing to files requires being in PASSIVE_LEVEL.
		// 

		auto const OldIrql = KeGetCurrentIrql();

		if (OldIrql != PASSIVE_LEVEL)
			KeLowerIrql(PASSIVE_LEVEL);

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
		// If we have to write the logs in ANSI format, then...
		// 

		if (this->ShouldStoreAsAnsi)
		{
			// 
			// Convert the UNICODE string to a ANSI string.
			// 

			UNICODE_STRING UnicodeMessage;
			UnicodeMessage.Buffer = (PWCH) InMessage;
			UnicodeMessage.Length = (USHORT) wcslen(InMessage) * sizeof(WCHAR);
			UnicodeMessage.MaximumLength = UnicodeMessage.Length + (1 * sizeof(WCHAR));
			
			ANSI_STRING AnsiMessage;
			AnsiMessage.Buffer = (PCHAR) ExAllocatePoolZero(NonPagedPoolNx, UnicodeMessage.MaximumLength, 'Log ');
			AnsiMessage.Length = 0;
			AnsiMessage.MaximumLength = UnicodeMessage.MaximumLength;
			
			if (NT_SUCCESS(RtlUnicodeStringToAnsiString(&AnsiMessage, &UnicodeMessage, false)))
			{
				// 
				// Write the message to a file on disk.
				// 

				IO_STATUS_BLOCK IoStatusBlock = { };

				if (NT_SUCCESS(ZwWriteFile(FileHandle, NULL, NULL, NULL, &IoStatusBlock, AnsiMessage.Buffer, AnsiMessage.Length, NULL, NULL)))
					ZwFlushBuffersFile(FileHandle, &IoStatusBlock);
			}

			// 
			// Release the memory allocated for the conversion.
			// 

			ExFreePoolWithTag(AnsiMessage.Buffer, 'Log ');
		}
		else
		{
			// 
			// Calculate the length of message.
			// 

			UNICODE_STRING UnicodeMessage;
			UnicodeMessage.Buffer = (PWCH) InMessage;
			UnicodeMessage.Length = (USHORT) wcslen(InMessage) * sizeof(WCHAR);
			UnicodeMessage.MaximumLength = UnicodeMessage.Length + (1 * sizeof(WCHAR));
			
			// 
			// Write the message to a file on disk.
			// 

			IO_STATUS_BLOCK IoStatusBlock = { };

			if (NT_SUCCESS(ZwWriteFile(FileHandle, NULL, NULL, NULL, &IoStatusBlock, UnicodeMessage.Buffer, UnicodeMessage.Length, NULL, NULL)))
				ZwFlushBuffersFile(FileHandle, &IoStatusBlock);
		}

		// 
		// Revert the IRQL back to its original level.
		// 

		if (OldIrql != PASSIVE_LEVEL)
			KfRaiseIrql(OldIrql);
	}

public:
	
	/// <summary>
	/// Destroys this log provider.
	/// </summary>
	void Exit() override
	{
		// 
		// Close the handle if it is still open.
		// 

		if (this->FileHandle != nullptr)
		{
			ZwClose(this->FileHandle);
			this->FileHandle = nullptr;
		}
	}
};