#pragma once

#include <tinyxml2.h>

// Loads an XML file with a Unicode path into a tinyxml2 document (tinyxml2::XMLDocument::LoadFile
// takes a narrow path; this opens the file with _wfopen and hands over the FILE*)
class CXmlFile
{
public:
	// Loads path into doc; returns XML_SUCCESS, XML_ERROR_FILE_NOT_FOUND when the file cannot be
	// opened, or tinyxml2's parse error (doc.ErrorStr() and doc.ErrorLineNum() describe it)
	static tinyxml2::XMLError Load(tinyxml2::XMLDocument& doc, const CString& path);
};
