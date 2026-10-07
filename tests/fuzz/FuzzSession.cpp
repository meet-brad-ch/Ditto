/**
 * @file FuzzSession.cpp
 * @brief Implements FuzzSession.
 */
#include "FuzzSession.h"

#include "CfHtmlFuzzTarget.h"
#include "ClipTextFuzzTarget.h"
#include "DibFuzzTarget.h"
#include "HdropFuzzTarget.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

std::vector<std::unique_ptr<FuzzTarget>> FuzzSession::s_targets{};
const FuzzTarget* FuzzSession::s_selected{};

std::vector<std::unique_ptr<FuzzTarget>> FuzzSession::AllTargets()
{
	std::vector<std::unique_ptr<FuzzTarget>> targets;
	targets.push_back(std::make_unique<CfHtmlFuzzTarget>());
	targets.push_back(std::make_unique<ClipTextFuzzTarget>());
	targets.push_back(std::make_unique<DibFuzzTarget>());
	targets.push_back(std::make_unique<HdropFuzzTarget>());
	return targets;
}

int FuzzSession::Initialize()
{
	s_targets = AllTargets();
	SelectTarget(Environment("DITTO_FUZZ_TARGET"));

	const std::string seeds = Environment("DITTO_FUZZ_WRITE_SEEDS");
	if (!seeds.empty())
	{
		WriteSeeds(seeds);
		std::exit(0);
	}
	return 0;
}

std::string FuzzSession::Environment(const char* name)
{
	char value[1024]{};
	std::size_t length{};
	if (getenv_s(&length, value, sizeof(value), name) != 0 || length == 0)
	{
		return std::string();
	}
	return std::string(value);
}

void FuzzSession::SelectTarget(const std::string& name)
{
	for (const std::unique_ptr<FuzzTarget>& target : s_targets)
	{
		if (target->Name() == name)
		{
			s_selected = target.get();
			return;
		}
	}
	std::fprintf(stderr, "DITTO_FUZZ_TARGET must name a target:");
	for (const std::unique_ptr<FuzzTarget>& target : s_targets)
	{
		std::fprintf(stderr, " %.*s", static_cast<int>(target->Name().size()), target->Name().data());
	}
	std::fprintf(stderr, "\n");
	std::exit(1);
}

void FuzzSession::Run(std::span<const std::byte> input)
{
	s_selected->Run(input);
}

void FuzzSession::WriteSeeds(const std::filesystem::path& folder)
{
	std::filesystem::create_directories(folder);
	int index{};
	for (const std::vector<std::byte>& seed : s_selected->Seeds())
	{
		std::ofstream file(folder / ("seed-" + std::to_string(index++)), std::ios::binary);
		file.write(reinterpret_cast<const char*>(seed.data()), static_cast<std::streamsize>(seed.size()));
	}
}
