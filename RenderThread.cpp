#include "stdafx.h"
#include <process.h>
#include "config.h"

#include "RenderThread.h"

#include "AppInstance.h"
#include "AsynchTexLoader.h"
#include "Console.h"
#include "ScriptedCoverPositions.h"
#include "DisplayPosition.h"


void RenderThread::renderThreadProc(){
	onDeviceModeChange();
	while (!closeRenderThread){
		if (!unAttachDone.waitForSignal(0)){
			renderer.destroyGlWindow();
			unAttachDone.setSignal();
			continue;
		}
		if (attachData.requested){
			attachData.result = renderer.attachGlWindow();
			attachData.requested = false;
			attachData.done.setSignal();
			if (attachData.result) renderer.initGlState();
			continue;
		}
		if (multisamplingInit.requested){
			multisamplingInit.result = renderer.initMultisampling();
			multisamplingInit.requested = false;
			multisamplingInit.done.setSignal();
			continue;
		}

		if (getPosData.requested){
			getPosData.outFound = renderer.positionOnPoint(getPosData.x, getPosData.y, *getPosData.outPos);
			getPosData.requested = false;
			getPosData.done.setSignal();
			doPaint = true;
			continue;
		}
		if (shareListData.requested){
			renderer.freeRC();
			shareListData.rcFreed.setSignal();
			shareListData.requested = false;
			shareListData.shareDone.waitForSignal();
			renderer.takeRC();
			continue;
		}
		if (resizeData.requested){
			ScopeCS scopeLock(resizeDataCS);
			renderer.resizeGlScene(resizeData.width, resizeData.height);
			resizeData.requested = false;
			continue;
		}
		if (textFormatChanged){
			textFormatChanged = false;
			renderer.textDisplay.clearCache();
			continue;
		}
		if (doPaint || forceRedraw){
			onPaint();
			continue;
		}
		renderThreadHasWork.waitForSignal();
	}
}

void RenderThread::onPaint(){
	double frameStart = Helpers::getHighresTimer();
	appInstance->texLoader->blockUpload();
	appInstance->texLoader->runGlDelete();
	appInstance->displayPos->accessCS.enter();
	appInstance->displayPos->update();
	appInstance->texLoader->setQueueCenter(appInstance->displayPos->getTarget());

	appInstance->coverPos->lock();
	renderer.drawFrame();
	appInstance->coverPos->unlock();
	appInstance->displayPos->accessCS.leave();



	/*double curTime = Helpers::getHighresTimer();
	int sleepFor = int((1000.0/(refreshRate*1.10)) - (1000*(curTime - lastTime))); //1.05 are tolerance
	if (sleepFor > 0)
	SleepEx(sleepFor,false);*/

	double frameEnd = Helpers::getHighresTimer();
	renderer.fpsCounter.recordFrame(frameStart, frameEnd);


	if (appInstance->displayPos->isMoving()){
		doPaint = true;
	} else {
		doPaint = false;
	}

	renderer.ensureVSync(cfgVSyncMode != VSYNC_SLEEP_ONLY);
	if (cfgVSyncMode == VSYNC_AND_SLEEP || cfgVSyncMode == VSYNC_SLEEP_ONLY){
		double currentTime = Helpers::getHighresTimer();
										 // time we have        time we already have spend
		int sleepTime = static_cast<int>((1000.0/refreshRate) - 1000*(currentTime - afterLastSwap));
		if (cfgVSyncMode == VSYNC_AND_SLEEP)
			sleepTime -= 2 * timerResolution;
		else
			sleepTime -= timerResolution;
		if (sleepTime >= 0){
			if (!timerInPeriod){
				timeBeginPeriod(timerResolution);
				timerInPeriod = true;
			}
			Sleep(sleepTime);
		}
	}
	if (forceRedraw){
		forceRedrawCS.enter();
		renderer.swapBuffers();
		afterLastSwap = Helpers::getHighresTimer();
		forceRedraw = false;
		forceRedrawCS.leave();
	} else {
		renderer.swapBuffers();
		afterLastSwap = Helpers::getHighresTimer();
	}
	if (!doPaint && appInstance->displayPos->isMoving())
		doPaint = true; // MT safety - if during Sleep something chaned dPos target and set forceRedraw we might have lost the change
	if (!doPaint)
		appInstance->texLoader->allowUpload();
	if (!doPaint && timerInPeriod){
		timeEndPeriod(timerResolution);
		timerInPeriod = false;
	}
}



RenderThread::RenderThread(AppInstance* appInstance) 
: appInstance(appInstance), unAttachDone(true, true), renderer(appInstance),
	afterLastSwap(0){
	forceRedraw = false;
	attachData.requested = false;
	multisamplingInit.requested = false;
	resizeData.requested = false;
	getPosData.requested = false;
	shareListData.requested = false;

	textFormatChanged = false;
	doPaint = false;

	timerInPeriod = false;
	timerResolution = 10;
	TIMECAPS tc;
	if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) == TIMERR_NOERROR){
		timerResolution = min(max(tc.wPeriodMin, 1), tc.wPeriodMax);
	}

	startRenderThread();
}

RenderThread::~RenderThread(){
	IF_DEBUG(Console::println(L"Destroying RenderThread"));
	stopRenderThread();
}

void RenderThread::startRenderThread(){
	closeRenderThread = false;
	renderThread = (HANDLE)_beginthreadex(0,0,&(this->runRenderThread),(void*)this,0,0);
	//SetThreadPriority(renderThread, THREAD_PRIORITY_HIGHEST);
}

unsigned int WINAPI RenderThread::runRenderThread(void* lpParameter)
{
	reinterpret_cast<RenderThread*>(lpParameter)->renderThreadProc();
	return 0;
}

void RenderThread::stopRenderThread()
{
	IF_DEBUG(Console::println(L"Stopping Render Thread"));
	closeRenderThread = true;
	renderThreadHasWork.setSignal();
	WaitForSingleObject(renderThread,INFINITE);
	CloseHandle(renderThread);
	renderThread = 0;
}

bool RenderThread::getPositionOnPoint (int x, int y, CollectionPos& out){
	getPosData.x = x;
	getPosData.y = y;
	getPosData.outPos = &out;
	getPosData.requested = true;
	renderThreadHasWork.setSignal();
	getPosData.done.waitForSignal();
	
	return getPosData.outFound;
}

void RenderThread::redraw(){
	ScopeCS scopeLock(forceRedrawCS);
	forceRedraw = true;
	renderThreadHasWork.setSignal();
}

void RenderThread::onViewportChange(){
	ScopeCS scopeLock(resizeDataCS);
	resizeData.requested = true;
	// a bit of a hack, but resizeData will always be filled with the last resize, so that's fine.
}
void RenderThread::onWindowResize(int newWidth, int newHeight){
	ScopeCS scopeLock(resizeDataCS);
	resizeData.width = newWidth;
	resizeData.height = newHeight;
	resizeData.requested = true;
	renderThreadHasWork.setSignal();
}

void RenderThread::onDeviceModeChange(){
	DEVMODE dispSettings;
	ZeroMemory(&dispSettings,sizeof(dispSettings));
	dispSettings.dmSize=sizeof(dispSettings);

	if (0 != EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&dispSettings)){
		refreshRate = dispSettings.dmDisplayFrequency;
		if (refreshRate >= 100) // we do not need 100fps - 50 is enough
			refreshRate /= 2;
	}
}

void RenderThread::onTextFormatChanged(){
	textFormatChanged = true;
}

bool RenderThread::shareLists(HGLRC shareWith){
	shareListData.requested = true;
	renderThreadHasWork.setSignal();
	shareListData.rcFreed.waitForSignal();
	bool res = renderer.shareLists(shareWith);
	shareListData.shareDone.setSignal();
	return res;
}




bool RenderThread::attachToMainWindow(){
	attachData.requested = true;
	renderThreadHasWork.setSignal();
	attachData.done.waitForSignal();
	return attachData.result;
}


bool RenderThread::initMultisampling(){
	multisamplingInit.requested = true;
	renderThreadHasWork.setSignal();
	multisamplingInit.done.waitForSignal();
	return multisamplingInit.result;
}

void RenderThread::unAttachFromMainWindow(){
	unAttachDone.resetSignal();
	renderThreadHasWork.setSignal();
	unAttachDone.waitForSignal();
}
