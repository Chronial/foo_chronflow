#pragma once
#include "base.h"
#include "CriticalSection.h"

#define TEST_BIT_PTR(BITSET_PTR, BIT) _bittest(BITSET_PTR, BIT)
#ifdef _DEBUG
#define ASSUME(X) PFC_ASSERT(X)
#else
#define ASSUME(X) __assume(X)
#endif

#define RESTRICT_PTR __restrict
#define RESTRICT_FUNC __declspec(restrict)
#define NO_ALIAS __declspec(noalias)

void errorPopup(const char* message);
void errorPopupWin32(const char* message); // Display the given message and the GetLastError() info

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
		frameTimes[frameTimesP] = end;
		frameDur[frameTimesP] = thisFrame;
		if (++frameTimesP == 60)
			frameTimesP = 0;
	}
	
	void getFPS(double& fps, double& msPerFrame, double& longestFrame, double& minFps){
		double frameDurSum = 0;
		double frameTimesSum = 0;
		longestFrame = -1;
		double longestFrameTime = -1;
		double thisFrameTime;
		int frameTimesT = frameTimesP-1;
		if (frameTimesT < 0)
			frameTimesT = 59;

		double lastTime = frameTimes[frameTimesT];
		double endTime = lastTime;
		for (int i=0; i < 30; i++, frameTimesT--){
			if (frameTimesT < 0)
				frameTimesT = 59;

			frameDurSum += frameDur[frameTimesT];
			if (frameDur[frameTimesT] > longestFrame)
				longestFrame = frameDur[frameTimesT];

			thisFrameTime = lastTime - frameTimes[frameTimesT];
			if (thisFrameTime > longestFrameTime)
				longestFrameTime = thisFrameTime;
			lastTime = frameTimes[frameTimesT];
		}

		fps = 1/((endTime-lastTime) / 29);
		msPerFrame = frameDurSum * 1000 / 30;
		longestFrame *= 1000;
		minFps = 1 / longestFrameTime;
	}
};

class AppInstance;


#ifdef _DEBUG
#define CS_ASSERT(X) X.assertOwnage()
#else
#define CS_ASSERT(X)
#endif


volatile class ScopeCS {
	CriticalSection* criticalSection;
public:
	ScopeCS(CriticalSection& CS) {
		criticalSection = &CS;
		criticalSection->enter();
	}
	ScopeCS(CriticalSection* CS) {
		criticalSection = CS;
		criticalSection->enter();
	}
	~ScopeCS(){
		criticalSection->leave();
	}
};

class Event {
	HANDLE e;
public:
	Event(bool initialState = false, bool manualReset = false){
		e = CreateEvent(NULL,initialState,manualReset,NULL);
	}
	~Event(){
		CloseHandle(e);
	}
	inline bool waitForSignal(DWORD milliseconds = INFINITE){
		if (WAIT_OBJECT_0 == WaitForSingleObject(e, milliseconds)){
			return true;
		} else {
			return false;
		}
	}
	void setSignal(){
		SetEvent(e);
	}
	void resetSignal(){
		ResetEvent(e);
	}
	int waitForTwo(Event& other, bool waitForAll, DWORD milliseconds = INFINITE){
		HANDLE handles[2] = {this->e, other.e};
		return WaitForMultipleObjects(2, handles, waitForAll, milliseconds);
	}
};

template<typename T>
inline pfc::list_t<T> pfc_list(std::initializer_list<T> elems){
	pfc::list_t<T> out;
	out.prealloc(elems.size());
	for (auto& e : elems){
		out.add_item(e);
	}
	return out;
}
