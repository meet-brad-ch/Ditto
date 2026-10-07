#pragma once

#include <cstddef>
#include <vector>

// One row of the Data table: one clipboard format of a clip, by its registered name.
struct FormatRecord
{
	int dataId{ -1 };
	int parentId{ -1 };
	CString name{};
	std::vector<std::byte> data{};
};
