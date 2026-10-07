/**
 * @file FuzzSession.h
 * @brief Declares FuzzSession.
 */
#pragma once

#include "FuzzTarget.h"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

/**
 * @brief The fuzz target of this process, chosen once by the DITTO_FUZZ_TARGET environment variable.
 *
 * libFuzzer calls two C entry points; they forward here. With DITTO_FUZZ_WRITE_SEEDS=<dir>
 * the process only writes the target's seed corpus into that folder and exits.
 */
class FuzzSession
{
public:
	/**
	 * @brief Selects the target and, when asked, writes its seeds and exits the process.
	 * @return 0. Exits with 1 when DITTO_FUZZ_TARGET is missing or unknown.
	 */
	static int Initialize();

	/**
	 * @brief Runs the selected target on one input.
	 * @param input The bytes libFuzzer generated.
	 */
	static void Run(std::span<const std::byte> input);

private:
	/** @return Every target this binary knows. */
	static std::vector<std::unique_ptr<FuzzTarget>> AllTargets();

	/**
	 * @brief Reads an environment variable.
	 * @param name The variable.
	 * @return Its value, or empty when it is not set.
	 */
	static std::string Environment(const char* name);

	/**
	 * @brief Selects the target with the given name; lists the names and exits with 1 when none has it.
	 * @param name The target name from DITTO_FUZZ_TARGET.
	 */
	static void SelectTarget(const std::string& name);

	/**
	 * @brief Writes each seed of the selected target as seed-<n> into a folder.
	 * @param folder The corpus folder; created when missing.
	 */
	static void WriteSeeds(const std::filesystem::path& folder);

	/// The targets, owned for the whole process.
	static std::vector<std::unique_ptr<FuzzTarget>> s_targets;
	/// The selected target, one of s_targets.
	static const FuzzTarget* s_selected;
};
