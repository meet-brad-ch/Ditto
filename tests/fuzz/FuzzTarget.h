/**
 * @file FuzzTarget.h
 * @brief Declares FuzzTarget, the interface of one libFuzzer target.
 */
#pragma once

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

/**
 * @brief One parser under fuzzing.
 *
 * Run feeds arbitrary bytes to the parser. A ClipboardFormatError is the parser rejecting bad
 * input, which is correct; a crash, a sanitizer report or any other exception is a finding.
 */
class FuzzTarget
{
public:
	virtual ~FuzzTarget() = default;

	/** @return The name selected by DITTO_FUZZ_TARGET. */
	virtual std::string_view Name() const = 0;

	/**
	 * @brief Runs the parser on one input.
	 * @param input The bytes libFuzzer generated.
	 */
	virtual void Run(std::span<const std::byte> input) const = 0;

	/** @return Valid inputs to start the corpus from. */
	virtual std::vector<std::vector<std::byte>> Seeds() const = 0;
};
