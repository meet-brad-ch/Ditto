#pragma once
#include "EventThread.h"
#include "Clip.h"
#include <afxmt.h>
#include <memory>

class CMainFrmThread : public CEventThread
{
public:
    CMainFrmThread(void);
    ~CMainFrmThread(void);

    enum eCMainFrmThreadEvents
    {
        DELETE_ENTRIES, 
        REMOVE_TEMP_FILES, 
		SAVE_CLIPS,
		READ_DB_FILE,

        ECMAINFRMTHREADEVENTS_COUNT  //must be last
    };

    void FireDeleteEntries() { FireEvent(DELETE_ENTRIES); }
    void FireRemoveTempFiles() { FireEvent(REMOVE_TEMP_FILES); }
	void FireReadDbFile() { FireEvent(READ_DB_FILE); }

	// Queues a copied clip for saving on this thread; the thread owns it from now on
	void AddClipToSave(std::unique_ptr<CClip> clip);

protected:
    virtual void OnEvent(int eventId, void *param);

    void OnDeleteEntries();
    void OnRemoveTempFiles();
	void OnSaveClips();
	void OnReadDbFile();

	CCriticalSection m_cs;
	CClipList m_saveClips;
};
