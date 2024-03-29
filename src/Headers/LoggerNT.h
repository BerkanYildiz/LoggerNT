#pragma once

// 
// Include the basic windows kernel headers.
// 

#include <ntdef.h>
#include <ntifs.h>
#include <vadefs.h>
#include <wdm.h>
#include <ntstrsafe.h>

// 
// Define the library globals.
// 

#define LOGGER_NT_VERSION_MAJOR 1
#define LOGGER_NT_VERSION_MINOR 1
#define LOGGER_NT_VERSION_BUILD 0
#define LOGGER_NT_POOL_TAG 0

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
