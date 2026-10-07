#include "stdafx.h"
#include "XmlFile.h"

#include <cstdio>
#include <memory>

tinyxml2::XMLError CXmlFile::Load(tinyxml2::XMLDocument& doc, const CString& path)
{
	FILE* opened{};
	if (_wfopen_s(&opened, path, L"rb") != 0 || opened == nullptr)
	{
		return tinyxml2::XML_ERROR_FILE_NOT_FOUND;
	}
	const std::unique_ptr<FILE, decltype(&fclose)> file{ opened, &fclose };
	return doc.LoadFile(file.get());
}
