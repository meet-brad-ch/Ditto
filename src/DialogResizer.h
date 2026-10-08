// DialogResizer.h: interface for the CDialogResizer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DIALOGRESIZER_H__DA9AF3FF_C6CC_4D70_965A_4216A0EC9E75__INCLUDED_)
#define AFX_DIALOGRESIZER_H__DA9AF3FF_C6CC_4D70_965A_4216A0EC9E75__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxtempl.h>

class CDialogResizer
{
public:
	/** @brief How a control follows the dialog's size (AddControl's flags, combined with |). */
	enum : int
	{
		/** @brief The control moves with the right edge. */
		MoveLeft = 1,
		/** @brief The control moves with the bottom edge. */
		MoveTop = 2,
		/** @brief The control's width follows the dialog's width. */
		SizeWidth = 4,
		/** @brief The control's height follows the dialog's height. */
		SizeHeight = 8,
	};

	CDialogResizer();
	virtual ~CDialogResizer();

protected:
	class CDR_Data
	{
	public:
		CDR_Data()
		{
			m_nFlags = 0;
		}
		HWND m_hWnd{};
		int m_nFlags;
	};

public:
	void MoveControls(CSize csNewSize);

	void AddControl(int nControlID, int nFlags);
	void AddControl(HWND hWnd, int nFlags);

	void SetParent(HWND hWndParent);

protected:
	CArray<CDR_Data, CDR_Data> m_Controls;
	CSize m_DlgSize;
	HWND m_hWndParent{};

protected:
};

#endif // !defined(AFX_DIALOGRESIZER_H__DA9AF3FF_C6CC_4D70_965A_4216A0EC9E75__INCLUDED_)
