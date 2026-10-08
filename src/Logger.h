#pragma once

#include <source_location>

/** @brief Writes log lines tagged with the source file and line of the caller. */
class CLogger
{
public:
	/**
	 * @brief Logs a message with the caller's file and line (see Write()).
	 * @param msg The message.
	 * @param location The caller's position; leave it to the default.
	 */
	static void Log(const TCHAR* msg, const std::source_location location = std::source_location::current())
	{
		Write(msg, CString(location.file_name()), static_cast<long>(location.line()));
	}

	/**
	 * @brief Writes a time-stamped log line "[date time - file line] msg" to the debugger output
	 * (Release: when the settings' m_outputDebugStringLogging is set) and appends it to Ditto.log
	 * in the log folder (Release: when the settings' m_bEnableDebugLogging is set). Reads the
	 * settings through theApp.Services().Settings() (the documented exception to the access rule).
	 * @param msg The message.
	 * @param csFile The source file; only its file name is written.
	 * @param lLine The source line.
	 */
	static void Write(const TCHAR* msg, CString csFile = _T(""), long lLine = -1);

private:
	/**
	 * @brief Appends a text to a file (created when missing).
	 * @param fn The file.
	 * @param msg The text.
	 */
	static void AppendToFile(const TCHAR* fn, const TCHAR* msg);
};
