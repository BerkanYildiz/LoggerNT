#pragma once

struct LoggerConfig
{
public:
	
	/// <summary>
	/// The minimum level of severity for a log output to be transferred to the logging providers.
	/// </summary>
	ELogLevel MinimumLevel = ELogLevel::Trace;
};