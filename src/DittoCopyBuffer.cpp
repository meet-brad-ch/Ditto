#include "stdafx.h"
#include ".\dittocopybuffer.h"
#include "CP_Main.h"
#include "ErrorReport.h"
#include <Mmsystem.h> //play sound
#include <stdexcept>
#include <string>


CDittoCopyBuffer::CDittoCopyBuffer(CGetSetOptions& settings, CDittoDb& database, ExternalWindowTracker& activeWindow, CAppWindows& windows, const CRegisteredClipboardFormats& clipboardFormats) :
	m_settings(settings),
	m_database(database),
	m_activeWindow(activeWindow),
	m_windows(windows),
	m_clipboardFormats(clipboardFormats),
	m_SavedClipboard(windows, clipboardFormats),
	m_ActiveTimer(TRUE, TRUE),
	m_RestoreTimer(TRUE, TRUE),
	m_Pasting(TRUE, TRUE)
{
	m_bActive = false;
	m_dwLastPaste = 0;
}

CDittoCopyBuffer::~CDittoCopyBuffer(void)
{
}


bool CDittoCopyBuffer::StartCopy(long lCopyBuffer, bool bCut)
{
	CLogger::Log(CStringUtil::Format(_T("Start of Ditto Copy buffer = %d"), lCopyBuffer));

	//Tell the timer thread to exit
	m_ActiveTimer.SetEvent();
	//Make sure the end copy thread has exited
	EndRestoreThread();

	if (m_SavedClipboard.Save(FALSE))
	{
		if (bCut)
		{
			m_activeWindow.SendCut();
		}
		else
		{
			m_activeWindow.SendCopy(CopyReasonEnum::COPY_TO_BUFFER);
		}

		//Create a thread to track if they have copied anything, if thread has exited before they have
		//copied something then the copy buffer copy is canceled
		AfxBeginThread(CDittoCopyBuffer::StartCopyTimer, (LPVOID)this, THREAD_PRIORITY_LOWEST);

		m_bActive = true;
		m_lCurrentDittoBuffer = lCopyBuffer;
	}
	else
	{
		CLogger::Log(_T("Start of Ditto Failed to save buffer"));
	}

	return true;
}

UINT CDittoCopyBuffer::StartCopyTimer(LPVOID pParam)
{
	CDittoCopyBuffer* pBuffer = (CDittoCopyBuffer*)pParam;
	if (pBuffer)
	{
		pBuffer->m_ActiveTimer.ResetEvent();

		DWORD dRes = WaitForSingleObject(pBuffer->m_ActiveTimer, 1500);
		if (dRes == WAIT_TIMEOUT)
		{
			pBuffer->m_SavedClipboard.Clear();
			pBuffer->m_bActive = false;
		}
	}

	return 0;
}

bool CDittoCopyBuffer::EndCopy(long lID)
{
	if (m_lCurrentDittoBuffer < 0 || m_lCurrentDittoBuffer >= 10)
	{
		CLogger::Log(_T("tried to save copy buffer but copy buffer is empty"));
		return false;
	}

	if (m_bActive == false)
	{
		CLogger::Log(_T("Current buffer is not active can't save copy buffer to db"));
		return false;
	}

	m_ActiveTimer.SetEvent();
	m_bActive = false;

	CLogger::Log(CStringUtil::Format(_T("Start - Ditto EndCopy buffer = %d"), m_lCurrentDittoBuffer));

	bool bRet = false;

	//put the data that we stored at the start of this action back on the standard clipboard
	m_SavedClipboard.Restore();

	if (PutClipOnDittoCopyBuffer(m_settings, m_database, lID, m_lCurrentDittoBuffer))
	{
		CLogger::Log(CStringUtil::Format(_T("Ditto end copy, saved clip successfully Clip ID = %d"), lID));

		bRet = true;
	}
	else
	{
		CLogger::Log(CStringUtil::Format(_T("Ditto end copy, ERROR associating clip to Copy buffer ID = %d"), lID));
	}

	return bRet;
}

bool CDittoCopyBuffer::PutClipOnDittoCopyBuffer(CGetSetOptions& settings, CDittoDb& database, long lClipId, long lBuffer)
{
	try
	{
		//enclose in brackets so the query closes before we update below
		{
			CppSQLite3Query q = database.execQueryEx(_T("SELECT lID FROM CopyBuffers WHERE lCopyBuffer = %d"), lBuffer);
			if (q.eof())
			{
				database.execDMLEx(_T("INSERT INTO CopyBuffers VALUES(NULL, -1, %d);"), lBuffer);
			}
		}

		database.execDMLEx(_T("UPDATE CopyBuffers SET lClipID = %d WHERE lCopyBuffer = %d"), lClipId, lBuffer);

		CCopyBufferItem Item;
		settings.GetCopyBufferItem(lBuffer, Item);
		if (Item.m_bPlaySoundOnCopy)
		{
			PlaySound(_T("ding.wav"), NULL, SND_FILENAME | SND_ASYNC);
		}

		return true;
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Saving clip id %ld to Ditto copy buffer %ld failed: %s"), lClipId, lBuffer, e.errorMessage()));
		return false;
	}
}

bool CDittoCopyBuffer::PastCopyBuffer(long lCopyBuffer)
{
	//Can't paste while another is still active
	if (WaitForSingleObject(m_Pasting, 1) == WAIT_TIMEOUT)
	{
		CLogger::Log(_T("Copy Buffer pasted to fast"));
		return false;
	}

	m_RestoreTimer.ResetEvent();
	m_Pasting.ResetEvent();
	bool bRet = false;

	CLogger::Log(CStringUtil::Format(_T("Start - PastCopyBuffer buffer = %d"), m_lCurrentDittoBuffer));

	try
	{
		CppSQLite3Query q = m_database.execQueryEx(_T("SELECT Main.lID FROM Main ")
												   _T("INNER JOIN CopyBuffers ON CopyBuffers.lClipID = Main.lID ")
												   _T("WHERE CopyBuffers.lCopyBuffer = %d"),
												   lCopyBuffer);

		if (q.eof() == false)
		{
			m_pClipboard = std::make_unique<CClipboardSaveRestoreCopyBuffer>(m_windows, m_clipboardFormats);
			//Save the clipboard,
			//then put the new data on the clipboard
			//then send a paste
			//then wait a little and restore the original clipboard data
			if (m_pClipboard->Save(false))
			{
				m_windows.MainFrame()->PasteOrShowGroup(q.getIntField(_T("lID")), -1, FALSE, TRUE, false);

				m_pClipboard->m_lRestoreDelay = m_settings.GetDittoRestoreClipboardDelay();

				CLogger::Log(CStringUtil::Format(_T("PastCopyBuffer sent paste, starting thread to restore clipboard, Delay = %d"), m_pClipboard->m_lRestoreDelay));

				// the thread takes m_pClipboard over; this thread does not touch it until m_Pasting is set
				if (AfxBeginThread(CDittoCopyBuffer::DelayRestoreClipboard, (LPVOID)this, THREAD_PRIORITY_LOWEST) == NULL)
				{
					// without the thread the saved clipboard is never restored and no later paste could start
					m_pClipboard.reset();
					CErrorReport::Show(CStringUtil::Format(_T("Ditto copy buffer %ld was pasted, but the clipboard it replaced cannot be restored: the restore thread did not start"), lCopyBuffer));
				}
				else
				{
					bRet = true;
				}
			}
			else
			{
				CLogger::Log(_T("PastCopyBuffer failed to save clipboard"));
			}
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Pasting Ditto copy buffer %ld failed: %s"), lCopyBuffer, e.errorMessage()));
		m_Pasting.SetEvent(); // no paste is running, so the next one may start
		return false;
	}

	if (bRet == false)
		m_Pasting.SetEvent();

	return bRet;
}

void CDittoCopyBuffer::EndRestoreThread()
{
	//Tell the thread to stop waiting and restore the clipboard
	m_RestoreTimer.SetEvent();

	//make sure it's ended
	WaitForSingleObject(m_Pasting, 5000);
}

UINT CDittoCopyBuffer::DelayRestoreClipboard(LPVOID pParam)
{
	CDittoCopyBuffer* pBuffer = (CDittoCopyBuffer*)pParam;
	if (pBuffer)
	{
		// owns the clipboard saved by PastCopyBuffer from now on
		std::unique_ptr<CClipboardSaveRestoreCopyBuffer> pLocalClipboard{ std::move(pBuffer->m_pClipboard) };

		// signaled (EndRestoreThread) and timed out both mean: restore now. A failed wait is shown
		// and the restore skipped; upstream threw out of the thread, which ended the process
		BOOL restored{ FALSE };
		if (WaitForSingleObject(pBuffer->m_RestoreTimer, pLocalClipboard->m_lRestoreDelay) == WAIT_FAILED)
		{
			CErrorReport::Show(CStringUtil::Format(_T("The clipboard was not restored after the copy buffer paste: waiting for the restore delay failed, error %lu"), ::GetLastError()));
		}
		else if (GetKeyState(VK_SHIFT) & 0x8000)
		{
			CLogger::Log(_T("Shift key is down not restoring clipboard, custom Buffer on normal clipboard"));
			restored = TRUE;
		}
		else if (pLocalClipboard->Restore())
		{
			CLogger::Log(_T("CDittoCopyBuffer::DelayRestoreClipboard Successfully"));
			restored = TRUE;
		}
		else
		{
			// shown: upstream only logged it, so the copy buffer's clip silently stayed on the clipboard
			CErrorReport::Show(_T("The clipboard could not be restored after the copy buffer paste; the copy buffer's clip is still on it."));
		}

		// freed before the next paste may start
		pLocalClipboard.reset();

		pBuffer->m_Pasting.SetEvent();
		return restored;
	}

	return TRUE;
}
