#pragma once
#include "Clip.h"

class CSpecialPasteOptions
{
public:
	CSpecialPasteOptions();
	~CSpecialPasteOptions();

	bool m_pasteAsPlainText;
	bool m_pasteUpperCase;
	bool m_pasteCamelCase;
	bool m_pasteImagesHorizontal;
	bool m_pasteImagesVertically;
	bool m_pasteLowerCase;
	bool m_pasteCapitalize;
	bool m_pasteSentenceCase;
	bool m_pasteRemoveLineFeeds;
	bool m_pasteAddOneLineFeed;
	bool m_pasteAddTwoLineFeeds;
	bool m_pasteTypoglycemia;
	bool m_pasteAddingDateTime;
	CClipFormats* m_pPasteFormats;
	bool m_dragDropFilesOnly;
	bool m_updateClipOrder;
	bool m_trimWhiteSpace;
	bool m_PosixifyPaths;
	bool m_pasteSlugify;
	bool m_invertCase;
	bool m_placeCF_HDROP_OnDrag;
	bool m_pasteAsciiOnly;
	bool m_pasteGuid;
	bool m_pasteAsImage;

	bool LimitFormatsToText()
	{
		return IsPlainTextOrCaseChange() ||
			   IsLineFeedOrInsertChange() ||
			   IsTextRewrite();
	}

	bool IncludeRTFForTextOnly()
	{
		return m_pasteRemoveLineFeeds ||
			   m_pasteAddOneLineFeed ||
			   m_pasteAddTwoLineFeeds ||
			   m_pasteAddingDateTime;
	}

	CString ToString();

private:
	/**
	 * @brief LimitFormatsToText's first group: plain text or a case change.
	 * @return True when plain text, upper, lower, capitalize or sentence case is set.
	 */
	bool IsPlainTextOrCaseChange() const
	{
		return m_pasteAsPlainText ||
			   m_pasteUpperCase ||
			   m_pasteLowerCase ||
			   m_pasteCapitalize ||
			   m_pasteSentenceCase;
	}

	/**
	 * @brief LimitFormatsToText's second group: line feed changes, typoglycemia or an added date/time.
	 * @return True when one of these options is set.
	 */
	bool IsLineFeedOrInsertChange() const
	{
		return m_pasteRemoveLineFeeds ||
			   m_pasteAddOneLineFeed ||
			   m_pasteAddTwoLineFeeds ||
			   m_pasteTypoglycemia ||
			   m_pasteAddingDateTime;
	}

	/**
	 * @brief LimitFormatsToText's third group: the text rewrites (trim, posix paths, slugify, invert, camel case, ASCII only).
	 * @return True when one of these options is set.
	 */
	bool IsTextRewrite() const
	{
		return m_trimWhiteSpace ||
			   m_PosixifyPaths ||
			   m_pasteSlugify ||
			   m_invertCase ||
			   m_pasteCamelCase ||
			   m_pasteAsciiOnly;
	}
};
