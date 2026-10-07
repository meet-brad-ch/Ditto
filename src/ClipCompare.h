#pragma once
#include "Clip.h"
#include <array>

class CClipCompare
{
public:
	CClipCompare(void);
	~CClipCompare(void);

	void Compare(int leftId, int rightId);

protected:
	CString SaveToFile(int id, CClip *clip, bool saveW, bool SaveA, bool saveUtf8);
	CString GetComparePath(CString &params);

private:
	/** @brief Which text formats Compare writes to the compare files. */
	struct CompareFormats
	{
		/** @brief Write the unicode text. */
		bool saveW{true};
		/** @brief Write the CF_TEXT text. */
		bool saveA{true};
		/** @brief Write the unicode text as UTF-8. */
		bool saveUtf8{true};
	};

	/** @brief A compare application looked for when no diff app is configured. */
	struct CompareApp
	{
		/** @brief Path of the application executable. */
		const TCHAR* path;
		/** @brief Command line put before the two file names, nullptr for none. */
		const TCHAR* params;
		/** @brief true if path holds environment variables (resolved with CGetSetOptions::ResolvePath). */
		bool resolvePath;
	};

	/** @brief The compare applications in search order (first one found is used). */
	static constexpr std::array<CompareApp, 18> m_compareApps
	{{
		{ _T("C:\\Program Files\\Beyond Compare 5\\BCompare.exe"), nullptr, false },
		{ _T("C:\\Program Files (x86)\\Beyond Compare 4\\BCompare.exe"), nullptr, false },
		{ _T("C:\\Program Files\\Beyond Compare 4\\BCompare.exe"), nullptr, false },
		{ _T("C:\\Program Files (x86)\\Beyond Compare 3\\BCompare.exe"), nullptr, false },
		{ _T("C:\\Program Files\\Beyond Compare 3\\BCompare.exe"), nullptr, false },
		{ _T("C:\\Program Files (x86)\\WinMerge\\WinMergeU.exe"), nullptr, false },
		{ _T("C:\\Program Files\\WinMerge\\WinMergeU.exe"), nullptr, false },
		{ _T("C:\\Program Files (x86)\\Araxis\\Araxis Merge\\compare.exe"), nullptr, false },
		{ _T("C:\\Program Files\\Araxis\\Araxis Merge\\compare.exe"), nullptr, false },
		{ _T("C:\\Program Files (x86)\\Perforce\\p4merge.exe"), nullptr, false },
		{ _T("C:\\Program Files\\Perforce\\p4merge.exe"), nullptr, false },
		{ _T("c:\\Program Files\\totalcmd\\totalcmd64.exe"), _T(" /S=C "), false },
		{ _T("c:\\Program Files (x86)\\totalcmd\\totalcmd.exe"), _T(" /S=C "), false },
		{ _T("c:\\totalcmd\\totalcmd64.exe"), _T(" /S=C "), false },
		{ _T("c:\\totalcmd\\totalcmd.exe"), _T(" /S=C "), false },
		{ _T("%localappdata%\\Programs\\Microsoft VS Code\\Code.exe"), _T(" --diff "), true },
		{ _T("C:\\Program Files\\Microsoft VS Code\\Code.exe"), _T(" --diff "), false },
		{ _T("C:\\Program Files (x86)\\Microsoft VS Code\\Code.exe"), _T(" --diff "), false },
	}};

	/**
	 * @brief Decides which text formats both clips have, for the compare files.
	 * @param leftClip the left clip (formats loaded).
	 * @param rightClip the right clip (formats loaded).
	 * @return the formats to write.
	 */
	CompareFormats GetCompareFormats(CClip& leftClip, CClip& rightClip);
	/**
	 * @brief Compare's step after both clips loaded: compares them if they have a common text format.
	 * @param leftId the left clip id.
	 * @param leftClip the left clip.
	 * @param rightId the right clip id.
	 * @param rightClip the right clip.
	 */
	void CompareClips(int leftId, CClip& leftClip, int rightId, CClip& rightClip);
	/**
	 * @brief Writes both clips to files and starts the compare application (or tells there is none).
	 * @param leftId the left clip id.
	 * @param leftClip the left clip.
	 * @param rightId the right clip id.
	 * @param rightClip the right clip.
	 * @param formats the text formats to write.
	 */
	void LaunchCompare(int leftId, CClip& leftClip, int rightId, CClip& rightClip, const CompareFormats& formats);
	/**
	 * @brief Sets the command line for a configured diff application (Total Commander, VS Code).
	 * @param path the configured diff application path, lower case.
	 * @param params receives the command line; unchanged for other applications.
	 */
	static void SetConfiguredAppParams(const CString& path, CString& params);
};

