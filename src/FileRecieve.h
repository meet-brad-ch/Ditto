#pragma once

// Collects file paths and builds CF_HDROP data from them. Local only: the
// network file transfer that used to live here was removed in this fork.
class CFileRecieve
{
public:
	CFileRecieve();
	virtual ~CFileRecieve();

	HGLOBAL CreateCF_HDROPBuffer();

	HGLOBAL CreateCF_HDROPBufferAsString();

	void AddFile(CString csFile)	{ m_RecievedFiles.Add(csFile); }

protected:
	CStringArray m_RecievedFiles;
};
