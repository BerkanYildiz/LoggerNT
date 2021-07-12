#pragma once

/// <summary>
/// The base interface every logging providers must implement and inherit from.
/// </summary>
__interface ILogProvider
{
	/// <summary>
	/// Logs a message of the specified severity.
	/// </summary>
	/// <param name="InLogLevel">The severity.</param>
	/// <param name="InMessage">The message.</param>
	void Log(ELogLevel InLogLevel, const WCHAR* InMessage);

	/// <summary>
	/// Destroys this log provider.
	/// </summary>
	/// <param name="InLogger">The logger.</param>
	void Exit(CONST class Logger* InLogger);
};