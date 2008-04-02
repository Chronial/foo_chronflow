#pragma once

class Helpers
{
public:
	// Returns the time in seconds with maximum resolution
	static bool isPerformanceCounterSupported();
	static double getHighresTimer(void);
	static void fixPath(pfc::string_base & path);
	//static void FPS(HWND hWnd, CollectionPos pos, float offset);
};



class FpsCounter
{
	double frameTimes[60];
	double frameDur[60];
	int frameTimesP;

public:
	FpsCounter() : frameTimesP(0) {
		ZeroMemory(frameTimes, sizeof(frameTimes));
	}

	void recordFrame(double start, double end){
		double thisFrame = end - start;
		frameTimes[frameTimesP] = Helpers::getHighresTimer();
		frameDur[frameTimesP] = thisFrame;
		if (++frameTimesP == 60)
			frameTimesP = 0;
	}
	
	void getFPS(double& fps, double& msPerFrame, double& longestFrame){
		double frameDurSum = 0;
		double frameTimesSum = 0;
		longestFrame = -1;
		int frameTimesT = frameTimesP;
		double lastTime = frameTimes[frameTimesT];
		for (int i=0; i < 30; i++){
			frameTimesT--;
			if (frameTimesT < 0)
				frameTimesT = 59;
			frameDurSum += frameDur[frameTimesT];
			if (frameDur[frameTimesT] > longestFrame)
				longestFrame = frameDur[frameTimesT];
			frameTimesSum += frameTimes[frameTimesT] - lastTime;
			lastTime = frameTimes[frameTimesT];
		}
		fps = 1/(frameTimesSum / 30);
		msPerFrame = frameDurSum * 1000 / 30;
		longestFrame *= 1000;
	}
};

class AppInstance;

class SwapBufferTimer {
	HANDLE timerQueue;
	HANDLE timer;
	bool swapQueued;
	HWND redrawWindow;
	bool redrawQueued;
	double lastSwap;
	LockedRenderingContext* lockedRC;
	int refreshRate;
	UINT multimediaTimerRes;
	CRITICAL_SECTION syncCS;
	AppInstance* appInstance;
public:
	SwapBufferTimer(AppInstance* instance, HWND redrawWnd, LockedRenderingContext* swapRC)
		: redrawWindow(redrawWnd), lockedRC(swapRC), multimediaTimerRes(1), refreshRate(100),
		  swapQueued(false), lastSwap(0), appInstance(instance) {
			timerQueue = CreateTimerQueue();
			TIMECAPS tc;
			if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) == TIMERR_NOERROR){
				multimediaTimerRes = min(max(tc.wPeriodMin, 1), tc.wPeriodMax);
			}
			InitializeCriticalSectionAndSpinCount(&syncCS, 0x80000400);
			reloadRefreshRate();
	}
	~SwapBufferTimer(){
		HANDLE deleteTimerQueueEvent = CreateEvent(NULL, false, false, NULL);
		DeleteTimerQueueEx(timerQueue, deleteTimerQueueEvent);
		WaitForSingleObject(deleteTimerQueueEvent, INFINITE);
		CloseHandle(deleteTimerQueueEvent);
		DeleteCriticalSection(&syncCS);
	}
	void reloadRefreshRate(){
		DEVMODE dispSettings;
		memset(&dispSettings,0,sizeof(dispSettings));
		dispSettings.dmSize=sizeof(dispSettings);

		if (EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&dispSettings)){
			refreshRate = dispSettings.dmDisplayFrequency;
		}
	}

	bool requestBeginDraw(){
		EnterCriticalSection(&syncCS);
		bool ret = true;
		if (swapQueued){
			redrawQueued = true;
			ValidateRect(redrawWindow,NULL);
			ret = false;
		}
		LeaveCriticalSection(&syncCS);
		return ret;
	}
	void queueSwap(bool redrawAfterwards){
		EnterCriticalSection(&syncCS);
		redrawQueued = redrawAfterwards;
		if (swapQueued)
			return;
		swapQueued = true;
		double frameTime = 1.0/refreshRate;
		double timeleft  = frameTime - (Helpers::getHighresTimer() - lastSwap);
		int waitFor = int(timeleft*1000) - 2;
		if (waitFor < (int)multimediaTimerRes)
			waitFor = multimediaTimerRes;
		timeBeginPeriod(multimediaTimerRes);
		bool res = 0 != CreateTimerQueueTimer(&timer, timerQueue, timerCallback, (void*)this, waitFor, 0, 0/*WT_EXECUTEINTIMERTHREAD*/);
		if (!res){
			pfc::string8 temp;
			temp << format_win32_error(GetLastError());

		}
		LeaveCriticalSection(&syncCS);
	}

// Stuff down here is called from some Timer Thread
private:
	static VOID CALLBACK timerCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired){
		reinterpret_cast<SwapBufferTimer*>(lpParameter)->onTimer();
	}
	void onTimer();
};