#pragma once

#include "EventThread.h"

class ExternalWindowTracker;

class CUAC_Thread : public CEventThread
{
public:
	/**
	 * @brief Creates the thread object for the events of a Ditto process.
	 * @param processId The Ditto process the events belong to.
	 * @param activeWindow The tracker that sends the paste, copy or cut in this process; must
	 *        outlive this object.
	 */
	CUAC_Thread(int processId, ExternalWindowTracker& activeWindow);
	~CUAC_Thread(void);

	enum eUacThreadEvents
	{
		UAC_PASTE, 
		UAC_COPY,
		UAC_CUT,
		UAC_EXIT,

		eUacThreadEvents_COUNT  //must be last
	};

	int m_processId;

	void FirePaste()
	{
		FireEvent(UAC_PASTE);
	}

	void FireCopy()
	{
		FireEvent(UAC_COPY);
	}

	void FireCut()
	{
		FireEvent(UAC_CUT);
	}

	void FireExit()
	{
		FireEvent(UAC_EXIT);
	}

	bool UACPaste();
	bool UACCopy();
	bool UACCut();

private:
	/** @brief The tracker that sends the paste, copy or cut (not owned). */
	ExternalWindowTracker& m_activeWindow;

	virtual void OnEvent(int eventId, void *param);
	virtual void OnTimeOut(void *param);
	CString EnumName(eUacThreadEvents e);
	bool StartProcess();
};

