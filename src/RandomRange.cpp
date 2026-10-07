#include "stdafx.h"
#include "RandomRange.h"

CRandomRange::CRandomRange() :
	m_engine(std::random_device{}())
{
}

int CRandomRange::Next(int low, int high)
{
	return std::uniform_int_distribution<int>(low, high)(m_engine);
}
