/**
 * @file HdropFuzzTarget.cpp
 * @brief Implements HdropFuzzTarget.
 */
#include "HdropFuzzTarget.h"

#include "ClipboardFormatError.h"
#include "FileDropList.h"
#include "GlobalFileDrop.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <type_traits>
#include <vector>

#include <windows.h>

void HdropFuzzTarget::Run(std::span<const std::byte> input) const
{
	try
	{
		const std::vector<std::wstring> paths = DittoCore::FileDropList::Parse(input.data(), input.size()).Paths();
		// Build must write a block that parses back to the same paths (a parsed path is never
		// empty and never holds a null, so Build accepts every parsed list)
		const std::vector<std::byte> built = DittoCore::FileDropList::Build(paths);
		if (DittoCore::FileDropList::Parse(built.data(), built.size()).Paths() != paths)
		{
			std::abort(); // a finding: Build and Parse disagree
		}
	}
	catch (const DittoCore::ClipboardFormatError&)
	{
		// rejected as malformed: the expected outcome for most inputs
	}

	if (input.empty())
	{
		return;
	}
	struct GlobalFreeDeleter
	{
		void operator()(void* block) const { ::GlobalFree(block); }
	};
	const std::unique_ptr<std::remove_pointer_t<HGLOBAL>, GlobalFreeDeleter> block(::GlobalAlloc(GMEM_MOVEABLE, input.size()));
	void* data = ::GlobalLock(block.get());
	if (data == nullptr)
	{
		return;
	}
	std::memcpy(data, input.data(), input.size());
	::GlobalUnlock(block.get());
	try
	{
		static_cast<void>(DittoCore::GlobalFileDrop::Read(block.get()).Paths().size());
	}
	catch (const DittoCore::ClipboardFormatError&)
	{
	}
}

std::vector<std::vector<std::byte>> HdropFuzzTarget::Seeds() const
{
	const std::wstring widePaths = std::wstring(L"C:\\Temp\\a.txt") + L'\0' + L"C:\\Users\\b\\c.png" + L'\0';
	std::string wideBytes(reinterpret_cast<const char*>(widePaths.data()), widePaths.size() * sizeof(wchar_t));
	const std::string ansiPaths = std::string("C:\\Temp\\a.txt") + '\0' + "D:\\x.bin" + '\0';
	return { Block(wideBytes, true), Block(ansiPaths, false), Block(std::string(), true) };
}

std::vector<std::byte> HdropFuzzTarget::Block(const std::string& pathBytes, bool wide)
{
	const std::uint32_t header[5]{ 20u, 0u, 0u, 0u, wide ? 1u : 0u };
	std::vector<std::byte> block(sizeof(header));
	std::memcpy(block.data(), header, sizeof(header));
	for (char c : pathBytes)
	{
		block.push_back(static_cast<std::byte>(c));
	}
	const std::size_t terminator = wide ? sizeof(wchar_t) : 1;
	block.insert(block.end(), terminator, std::byte{ 0 });
	return block;
}
