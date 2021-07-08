#pragma once

// 
// One of the new "developers" (they had to fill the quota for their diversity program) had sex with
// the compiler + some of the main windows headers files and gave it STDs so now you cannot compile without this define. :))
// 

#define _NO_CRT_STDIO_INLINE

// 
// Include the basic windows kernel headers.
// 

#include <ntdef.h>
#include <ntifs.h>
#include <vadefs.h>
#include <tchar.h>
#include <wdm.h>
#include <ntstrsafe.h>
#include <cstdarg>

// 
// Define the library globals.
// 

#define LOGGER_NT_VERSION_MAJOR 1
#define LOGGER_NT_VERSION_MINOR 0
#define LOGGER_NT_VERSION_BUILD 0

// 
// Include the library headers.
// 

#include "LogLevel.hpp"
#include "LogProvider.hpp"
#include "LoggerConfig.hpp"
#include "Logger.hpp"

// 
// Include the default logging providers.
// 

#include "Providers/DbgPrintProvider.hpp"
#include "Providers/TempFileProvider.hpp"
