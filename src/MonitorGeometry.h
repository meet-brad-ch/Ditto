#pragma once

/** @brief Monitor work areas, screen size and placing window rects on a monitor. */
class CMonitorGeometry
{
public:
	/**
	 * @brief The screen width (SM_CXSCREEN); half of it for the dual-monitor sizes 1280x480,
	 * 1600x600 and 2048x768.
	 * @return The width in pixels.
	 */
	static int GetScreenWidth();

	/**
	 * @brief The screen height (SM_CYSCREEN); half of it for the stacked-monitor sizes 640x960,
	 * 800x1200 and 1024x1536.
	 * @return The height in pixels.
	 */
	static int GetScreenHeight();

	/**
	 * @brief Ensures a window rect lies on a monitor: moves it into the work area of the nearest
	 * monitor (a rect without a nearest monitor is first centered on the primary monitor).
	 * @param pcrRect The rect; changed in place.
	 * @return TRUE when the rect was moved.
	 */
	static BOOL EnsureWindowVisible(CRect *pcrRect);

	/**
	 * @brief The work area of the primary monitor.
	 * @return The work area; empty when it cannot be read.
	 */
	static CRect DefaultMonitorRect();

	/**
	 * @brief The work area of the monitor nearest to a rect's top left corner (the primary monitor
	 * when there is none).
	 * @param rect The rect.
	 * @return The work area; empty when it cannot be read.
	 */
	static CRect MonitorRectFromRect(CRect rect);

	/**
	 * @brief A rect of the same size centered on the work area of the monitor nearest to its top
	 * left corner (the primary monitor when there is none).
	 * @param startingRect The rect.
	 * @return The centered rect.
	 */
	static CRect CenterRect(CRect startingRect);

	/**
	 * @brief A rect of the same size centered on another rect.
	 * @param startingRect The rect.
	 * @param outerRect The rect to center on.
	 * @return The centered rect.
	 */
	static CRect CenterRectFromRect(CRect startingRect, CRect outerRect);
};
