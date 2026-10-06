#include "FileDropList.h"
#include "DropBlockReader.h"

#include <utility>

namespace DittoCore
{
	FileDropList FileDropList::Parse(const void* data, std::size_t size)
	{
		return FileDropList(DropBlockReader(data, size).ReadPaths());
	}

	FileDropList::FileDropList(std::vector<std::wstring> paths)
		: m_paths(std::move(paths))
	{
	}
}
