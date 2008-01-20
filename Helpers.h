#pragma once

class Helpers
{
public:
	// Returns the time in seconds with maximum resolution
	static bool isPerformanceCounterSupported();
	static double getHighresTimer(void);
	static void FPS(HWND hWnd, CollectionPos pos, float offset);
};
