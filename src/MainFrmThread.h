#pragma once
#include "EventThread.h"
#include "Clip.h"
#include <afxmt.h>
#include <memory>

class CGetSetOptions;
class CIdleTime;
class CDittoDb;
class CClipboardMonitor;
class CAppWindows;

class CMainFrmThread : public CEventThread
{
public:
    /**
     * @brief Creates the (not yet started) background thread of the main frame.
     * @param settings The application's settings (retention, temp folders, database path);
     *        must outlive this object.
     * @param idleTime The user's idle time (the retention and the database read); must outlive this object.
     * @param database The clip database (the group name of a clip saved to a group); must outlive this object.
     * @param clipboard The clipboard monitor (told about saved clips); must outlive this object.
     * @param windows The application's windows (the main frame shows the saved-to-group message);
     *        must outlive this object.
     */
    CMainFrmThread(CGetSetOptions& settings, CIdleTime& idleTime, CDittoDb& database, CClipboardMonitor& clipboard, CAppWindows& windows);
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

	/// The application's settings (not owned).
	CGetSetOptions& m_settings;
	/// The user's idle time (not owned).
	CIdleTime& m_idleTime;
	/// The clip database (not owned).
	CDittoDb& m_database;
	/// The clipboard monitor (not owned).
	CClipboardMonitor& m_clipboard;
	/// The application's windows (not owned).
	CAppWindows& m_windows;
	CCriticalSection m_cs;
	CClipList m_saveClips;
};
