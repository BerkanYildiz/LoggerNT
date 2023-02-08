#pragma once

/// <summary>
/// The different levels of severity available for logging outputs.
/// </summary>
enum class ELogLevel : unsigned int
{
	Trace = 0,
	Debug = 1,
	Information = 2,
	Warning = 3,
	Error = 4,
	Fatal = 5,
	Disabled = 6,
};