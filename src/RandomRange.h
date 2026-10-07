#pragma once

#include "IRandomRange.h"

#include <random>

// DittoCore::IRandomRange over a Mersenne Twister seeded from std::random_device
class CRandomRange : public DittoCore::IRandomRange
{
public:
	CRandomRange();

	int Next(int low, int high) override;

private:
	std::mt19937 m_engine;
};
