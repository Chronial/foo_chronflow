#include "stdafx.h"
#include <process.h>
#include "config.h"

#include "RenderThread.h"

#include "AppInstance.h"
#include "AsynchTexLoader.h"
#include "Console.h"
#include "ScriptedCoverPositions.h"
#include "DisplayPosition.h"


void RenderThread::threadProc(){
	// Required that we can compile CoverPos Scripts
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	for (;;){
		if (messageQueue.size() == 0 && doPaint){
			onPaint();
			continue;
		}
		auto msg = messageQueue.pop();

		if (auto m = dynamic_pointer_cast<RTStopThreadMessage>(msg)){
			break;
		} else if (auto m = dynamic_pointer_cast<RTPaintMessage>(msg)){
			// do nothing, this is just here so that onPaint may run
		} else if (auto m = dynamic_pointer_cast<RTRedrawMessage>(msg)){
			doPaint = true;
		} else if (auto m = dynamic_pointer_cast<RTTargetChangedMessage>(msg)){
			collection_read_lock lock(appInstance);
			displayPos.onTargetChange();
		} else if (auto m = dynamic_pointer_cast<RTAttachMessage>(msg)){
			bool result = renderer.attachGlWindow();
			m->setAnswer(result);
			if (result){
				renderer.initGlState();
				texLoader.start();
			}
		} else if (auto m = dynamic_pointer_cast<RTUnattachMessage>(msg)){
			renderer.destroyGlWindow();
			m->setAnswer(true);
		} else if (auto m = dynamic_pointer_cast<RTShareListDataMessage>(msg)){
			renderer.freeRC();
			auto answer = make_shared<RTShareListDataAnswer>();
			m->setAnswer(answer);
			answer->getAnswer();
			renderer.takeRC();
		} else if (auto m = dynamic_pointer_cast<RTMultiSamplingMessage>(msg)){
			m->setAnswer(renderer.initMultisampling());
		} else if (auto m = dynamic_pointer_cast<RTDeviceModeMessage>(msg)){
			updateRefreshRate();
		} else if (auto m = dynamic_pointer_cast<RTWindowResizeMessage>(msg)){
			renderer.resizeGlScene(m->width, m->height);
		} else if (auto m = dynamic_pointer_cast<RTWindowHideMessage>(msg)){
			texLoader.pauseLoading();
			if (cfgEmptyCacheOnMinimize){
				texLoader.clearCache();
			}
		} else if (auto m = dynamic_pointer_cast<RTWindowShowMessage>(msg)){
			texLoader.resumeLoading();
		} else if (auto m = dynamic_pointer_cast<RTTextFormatChangedMessage>(msg)){
			renderer.textDisplay.clearCache();
		} else if (auto m = dynamic_pointer_cast<RTGetPosAtCoordsMessage>(msg)){
			collection_read_lock lock(appInstance);
			int offset;
			if (renderer.offsetOnPoint(m->x, m->y, offset)){
				CollectionPos pos = displayPos.getOffsetPos(offset);
				m->setAnswer(make_shared<CollectionPos>(pos));
			} else {
				m->setAnswer(nullptr);
			}
		} else if (auto m = dynamic_pointer_cast<RTChangeCPScriptMessage>(msg)){
			pfc::string8 tmp;
			renderer.coverPos.setScript(m->script, tmp);
			renderer.setProjectionMatrix();
			doPaint = true;
		} else if (auto m = dynamic_pointer_cast<RTCollectionReloadStartMessage>(msg)){
			// We will clear the cache anyways when we are done.
			// So stop texloader now to have more cpu for reload
			texLoader.pauseLoading();
		} else if (auto m = dynamic_pointer_cast<RTCollectionReloadedMessage>(msg)){
			boost::unique_lock<DbAlbumCollection> lock(*(appInstance->albumCollection));
			auto reloadWorker = std::move(m->worker);
			{
				// TODO: this is ugly, texloader should do this in its own thread?
				texLoader.clearCache();
				renderer.takeRC();
			}
			appInstance->albumCollection->onCollectionReload(*reloadWorker);
			CollectionPos newTargetPos = appInstance->albumCollection->getTargetPos();
			displayPos.hardSetCenteredPos(newTargetPos);
			texLoader.setQueueCenter(newTargetPos);
			texLoader.resumeLoading();
			appInstance->redrawMainWin();
		} else {
			IF_DEBUG(__debugbreak());
		}
	}
	CoUninitialize();
}

void RenderThread::send(shared_ptr<RTMessage> msg){
	messageQueue.push(msg);
}

void RenderThread::onPaint(){
	appInstance->albumCollection->lock_shared();
	double frameStart = Helpers::getHighresTimer();
	texLoader.blockUpload();

	displayPos.update();
	// continue animation if we are not done
	doPaint = displayPos.isMoving();
	texLoader.setQueueCenter(appInstance->albumCollection->getTargetPos());

	renderer.drawFrame();
	appInstance->albumCollection->unlock_shared();


	/*double curTime = Helpers::getHighresTimer();
	int sleepFor = int((1000.0/(refreshRate*1.10)) - (1000*(curTime - lastTime))); //1.05 are tolerance
	if (sleepFor > 0)
	SleepEx(sleepFor,false);*/

	double frameEnd = Helpers::getHighresTimer();
	renderer.fpsCounter.recordFrame(frameStart, frameEnd);



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

	renderer.swapBuffers();
	afterLastSwap = Helpers::getHighresTimer();

	if (doPaint){
		this->send(make_shared<RTPaintMessage>());
	} else {
		texLoader.allowUpload();
		if (timerInPeriod){
			timeEndPeriod(timerResolution);
			timerInPeriod = false;
		}

	}
}



RenderThread::RenderThread(AppInstance* appInstance)
	: appInstance(appInstance),
	  displayPos(appInstance, appInstance->albumCollection->begin()),
	  renderer(appInstance, &displayPos),
	  texLoader(appInstance),
	  afterLastSwap(0){
	renderer.texLoader = &texLoader;

	doPaint = false;

	timerInPeriod = false;
	timerResolution = 10;
	TIMECAPS tc;
	if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) == TIMERR_NOERROR){
		timerResolution = min(max(tc.wPeriodMin, 1), tc.wPeriodMax);
	}
	
	updateRefreshRate();
	renderThread = (HANDLE)_beginthreadex(0, 0, &(this->runRenderThread), (void*)this, 0, 0);
}

RenderThread::~RenderThread(){
	IF_DEBUG(Console::println(L"Destroying RenderThread"));
	stopRenderThread();
	texLoader.stopWorkerThread();
}


unsigned int WINAPI RenderThread::runRenderThread(void* lpParameter)
{
	reinterpret_cast<RenderThread*>(lpParameter)->threadProc();
	return 0;
}

void RenderThread::stopRenderThread()
{
	IF_DEBUG(Console::println(L"Stopping Render Thread"));
	this->send(make_shared<RTStopThreadMessage>());
	WaitForSingleObject(renderThread,INFINITE);
	CloseHandle(renderThread);
	renderThread = 0;
}

void RenderThread::updateRefreshRate(){
	DEVMODE dispSettings;
	ZeroMemory(&dispSettings,sizeof(dispSettings));
	dispSettings.dmSize=sizeof(dispSettings);

	if (0 != EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&dispSettings)){
		refreshRate = dispSettings.dmDisplayFrequency;
		if (refreshRate >= 100) // we do not need 100fps - 50 is enough
			refreshRate /= 2;
	}
}

bool RenderThread::shareLists(HGLRC shareWith){
	auto msg = make_shared<RTShareListDataMessage>();
	this->send(msg);
	auto answer = msg->getAnswer();
	bool res = renderer.shareLists(shareWith);
	answer->setAnswer(true);
	return res;
}