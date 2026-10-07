/**
 * @file FuzzMain.cpp
 * @brief The libFuzzer entry points; they forward to FuzzSession.
 */
#include "FuzzSession.h"

#include <cstddef>
#include <cstdint>

/**
 * @brief libFuzzer initialization callback: selects the target (DITTO_FUZZ_TARGET).
 * @param argc Unused.
 * @param argv Unused.
 * @return 0.
 */
extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv)
{
	static_cast<void>(argc);
	static_cast<void>(argv);
	return FuzzSession::Initialize();
}

/**
 * @brief libFuzzer callback for one input.
 * @param data The input bytes.
 * @param size The number of bytes.
 * @return 0.
 */
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
	FuzzSession::Run(std::span<const std::byte>(reinterpret_cast<const std::byte*>(data), size));
	return 0;
}
