#pragma once
#include "DbAlbumCollection.h"
#include "CriticalSection.h"
#include "Renderer.h"

class AppInstance;

class RenderThread {
	Renderer renderer;
	AppInstance* appInstance;
public:
	RenderThread(AppInstance* appInstance);
	~RenderThread();
	bool attachToMainWindow();
	bool initMultisampling();
	void unAttachFromMainWindow();

	bool getOffsetOnPoint(int x, int y, int& out); // synchronized
	void redraw(); // flag
	void onViewportChange();
	void onWindowResize(int newWidth, int newHeight); // flag
	void onDeviceModeChange();

	void onTextFormatChanged();

	bool shareLists(HGLRC shareWith); // frees RC in RenderThread, shared, retakes RC in RenderThread

	int getPixelFormat(){ return renderer.getPixelFormat(); };

	void stopRenderThread();
private:
	int timerResolution;
	bool timerInPeriod;
	int refreshRate;
	double afterLastSwap;

	void startRenderThread();
	static unsigned int WINAPI runRenderThread(void* lpParameter);
	bool closeRenderThread;
	void renderThreadProc();
	void onPaint();
	bool doPaint;
	HANDLE renderThread;
	Event renderThreadHasWork;

	bool textFormatChanged;

	struct __someNameA {
		volatile bool requested;
		volatile bool result;
		Event done;
	} attachData;

	Event unAttachDone;

	struct __someNameB {
		volatile bool requested;
		volatile bool result;
		Event done;
	} multisamplingInit;

	volatile bool forceRedraw;
	CriticalSection forceRedrawCS;

	struct __someNameC {
		volatile bool requested;
		volatile int width;
		volatile int height;
	} resizeData;
	CriticalSection resizeDataCS;

	struct __someNameD {
		volatile bool requested; // set this last - after all the other data is set!
		volatile int x;
		volatile int y;
		volatile bool outFound;
		int outOffset;
		Event done;
	} getOffsetData;

	struct __someNameE {
		volatile bool requested;
		Event rcFreed;
		Event shareDone; 
	} shareListData;
};
