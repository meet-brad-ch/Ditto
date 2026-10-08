#include "stdafx.h"
#include "MonitorGeometry.h"
#include <array>
#include <cstdint>
#include <cstdlib>

// Every supported Windows is NT. The former GetVersionEx check passed an OSVERSIONINFO without
// dwOSVersionInfoSize set, so it failed or read stack garbage and picked a branch by chance.
int CMonitorGeometry::GetScreenWidth()
{
	const int width{ GetSystemMetrics(SM_CXSCREEN) };
	const int height{ GetSystemMetrics(SM_CYSCREEN) };
	switch (width)
	{
	default: // also 640, 800 and 1024
		return (width);
	case 1280:
		if (height == 480)
		{
			return (width / 2);
		}
		return (width);
	case 1600:
		if (height == 600)
		{
			return (width / 2);
		}
		return (width);
	case 2048:
		if (height == 768)
		{
			return (width / 2);
		}
		return (width);
	}
}

int CMonitorGeometry::GetScreenHeight()
{
	const int width{ GetSystemMetrics(SM_CXSCREEN) };
	const int height{ GetSystemMetrics(SM_CYSCREEN) };
	switch (height)
	{
	default: // also 480, 600 and 768
		return (height);
	case 960:
		if (width == 640)
		{
			return (height / 2);
		}
		return (height);
	case 1200:
		if (width == 800)
		{
			return (height / 2);
		}
		return (height);
	case 1536:
		if (width == 1024)
		{
			return (height / 2);
		}
		return (height);
	}
}

CRect CMonitorGeometry::CenterRect(CRect startingRect)
{
	CRect crMonitor{};

	HMONITOR monitorHandle{ MonitorFromPoint(startingRect.TopLeft(), MONITOR_DEFAULTTONEAREST) };
	if (monitorHandle == NULL)
	{
		monitorHandle = MonitorFromPoint(startingRect.TopLeft(), MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO lpmi{};
		lpmi.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(monitorHandle, &lpmi))
		{
			crMonitor.CopyRect(&lpmi.rcWork);
		}
	}
	else
	{
		MONITORINFO lpmi{};
		lpmi.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(monitorHandle, &lpmi))
		{
			crMonitor.CopyRect(&lpmi.rcWork);
		}
	}

	return CenterRectFromRect(startingRect, crMonitor);
}

CRect CMonitorGeometry::CenterRectFromRect(CRect startingRect, CRect outerRect)
{
	const CPoint center{ outerRect.CenterPoint() };

	CRect centerRect{};

	centerRect.left = center.x - (startingRect.Width() / 2);
	centerRect.top = center.y - (startingRect.Height() / 2);
	centerRect.right = centerRect.left + startingRect.Width();
	centerRect.bottom = centerRect.top + startingRect.Height();

	return centerRect;
}

CRect CMonitorGeometry::DefaultMonitorRect()
{
	CRect crMonitor{};
	const CRect invalidRect(INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX);
	HMONITOR monitorHandle{ MonitorFromPoint(invalidRect.TopLeft(), MONITOR_DEFAULTTOPRIMARY) };
	MONITORINFO lpmi{};
	lpmi.cbSize = sizeof(MONITORINFO);
	if (GetMonitorInfo(monitorHandle, &lpmi))
	{
		crMonitor.CopyRect(&lpmi.rcWork);
	}

	return crMonitor;
}

CRect CMonitorGeometry::MonitorRectFromRect(CRect rect)
{
	CRect crMonitor{};

	HMONITOR monitorHandle{ MonitorFromPoint(rect.TopLeft(), MONITOR_DEFAULTTONEAREST) };
	if (monitorHandle == NULL)
	{
		monitorHandle = MonitorFromPoint(rect.TopLeft(), MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO lpmi{};
		lpmi.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(monitorHandle, &lpmi))
		{
			crMonitor.CopyRect(&lpmi.rcWork);
		}
	}
	else
	{
		MONITORINFO lpmi{};
		lpmi.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(monitorHandle, &lpmi))
		{
			crMonitor.CopyRect(&lpmi.rcWork);
		}
	}

	return crMonitor;
}

BOOL CMonitorGeometry::EnsureWindowVisible(CRect* pcrRect)
{
	BOOL ret{ FALSE };

	CRect crMonitor{};

	HMONITOR monitorHandle{ MonitorFromRect(pcrRect, MONITOR_DEFAULTTONEAREST) };
	if (monitorHandle == NULL)
	{
		monitorHandle = MonitorFromRect(pcrRect, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO lpmi{};
		lpmi.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(monitorHandle, &lpmi))
		{
			crMonitor.CopyRect(&lpmi.rcWork);

			*pcrRect = CenterRectFromRect(*pcrRect, crMonitor);
		}
	}
	else
	{
		MONITORINFO lpmi{};
		lpmi.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(monitorHandle, &lpmi))
		{
			crMonitor.CopyRect(&lpmi.rcWork);
		}
	}

	/** @brief One axis of a rect: its low edge (left or top) and its high edge (right or bottom). */
	struct Axis
	{
		/** @brief The low edge. */
		LONG RECT::* low{};
		/** @brief The high edge. */
		LONG RECT::* high{};
	};
	// horizontal first (left, right), then vertical (top, bottom)
	static constexpr std::array<Axis, 2> axes{ { { &RECT::left, &RECT::right }, { &RECT::top, &RECT::bottom } } };
	for (const Axis& axis : axes)
	{
		bool movedLow{ false };
		//Validate the left (top)
		long lDiff{ (*pcrRect).*axis.low - crMonitor.*axis.low };
		if (lDiff < 0)
		{
			(*pcrRect).*axis.low += abs(lDiff);
			(*pcrRect).*axis.high += abs(lDiff);
			ret = TRUE;
			movedLow = true;
		}

		//Right side (bottom)
		lDiff = (*pcrRect).*axis.high - crMonitor.*axis.high;
		if (lDiff > 0)
		{
			if (movedLow == false)
			{
				(*pcrRect).*axis.low -= abs(lDiff);
			}
			(*pcrRect).*axis.high -= abs(lDiff);
			ret = TRUE;
		}
	}

	return ret;
}
