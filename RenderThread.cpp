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
	TRACK_CALL_TEXT("Chronflow RenderThread");
	// Required that we can compile CoverPos Scripts
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	std::shared_ptr<RTInitDoneMessage> initDoneMsg;
	for (;;){
		// TODO: Improve this loop – separate this into startup and normal processing
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
			texLoader.send(make_shared<ATTargetChangedMessage>());
		} else if (auto m = dynamic_pointer_cast<RTInitDoneMessage>(msg)){
			glfwMakeContextCurrent(appInstance->glfwWindow);
			renderer.initGlState();
			// this blocks the mainthread till Texloader signals that he is done
			initDoneMsg = m;
			texLoader.start();
		} else if (auto m = dynamic_pointer_cast<RTTexLoaderStartedMessage>(msg)){
			initDoneMsg->setAnswer(true);
		} else if (auto m = dynamic_pointer_cast<RTDeviceModeMessage>(msg)){
			updateRefreshRate();
		} else if (auto m = dynamic_pointer_cast<RTWindowResizeMessage>(msg)){
			renderer.resizeGlScene(m->width, m->height);
		} else if (auto m = dynamic_pointer_cast<RTWindowHideMessage>(msg)){
			texLoader.send(make_shared<ATPauseMessage>());
			if (cfgEmptyCacheOnMinimize){
				texLoader.send(make_shared<ATClearCacheMessage>());
			}
		} else if (auto m = dynamic_pointer_cast<RTWindowShowMessage>(msg)){
			texLoader.send(make_shared<ATResumeMessage>());
		} else if (auto m = dynamic_pointer_cast<RTTextFormatChangedMessage>(msg)){
			renderer.textDisplay.clearCache();
		} else if (auto m = dynamic_pointer_cast<RTGetPosAtCoordsMessage>(msg)){
			collection_read_lock lock(appInstance);
			std::unique_lock<std::mutex> openglLock(texLoader.openglAccess);
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
		} else if (auto m = dynamic_pointer_cast<RTCollectionReloadedMessage>(msg)){
			auto tlMsg = make_shared<ATCollectionReloadMessage>();
			texLoader.send(tlMsg);
			tlMsg->waitForHalt();
			{
				// mainthread might be waiting for this thread (RTMessage.getAnswer()) and be holding a readlock
				// detect these cases by only trying for a short time
				// Maybe we can do this better, but that is difficult as we pass CollectionPos
				// pointers across thread boundaries and need to make sure they are valid
				boost::unique_lock<DbAlbumCollection> lock(*(appInstance->albumCollection), boost::defer_lock);
				// There is a race condition here, so use this only as a guess
				int waitTime = messageQueue.size() ? 10 : 100;
				if (lock.try_lock_for(boost::chrono::milliseconds(waitTime))){
					auto reloadWorker = appInstance->reloadWorker.synchronize();
					appInstance->albumCollection->onCollectionReload(**reloadWorker);
					CollectionPos newTargetPos = appInstance->albumCollection->getTargetPos();
					displayPos.hardSetCenteredPos(newTargetPos);
					reloadWorker->reset();
				} else {
					// looks like a deadlock, retry at the end of the messageQueue
					this->send(msg);
				}
			}
			tlMsg->allowThreadResume();
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
	TRACK_CALL_TEXT("RenderThread::onPaint");
	collection_read_lock lock(appInstance);
	std::unique_lock<std::mutex> openglLock(texLoader.openglAccess);
	double frameStart = Helpers::getHighresTimer();

	displayPos.update();
	// continue animation if we are not done
	doPaint = displayPos.isMoving();

	renderer.drawFrame();

	// this might not be right – do we need a glFinish() here?
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

	texLoader.trimCache();
	if (doPaint){
		this->send(make_shared<RTPaintMessage>());
	} else {
		if (timerInPeriod){
			timeEndPeriod(timerResolution);
			timerInPeriod = false;
		}
		// release before we notify texLoader
		openglLock.unlock();
		texLoader.send(make_shared<ATRenderingDoneMessage>());
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
		timerResolution = min(max(tc.wPeriodMin, (UINT)1), tc.wPeriodMax);
	}
	
	updateRefreshRate();
	renderThread = (HANDLE)_beginthreadex(0, 0, &(this->runRenderThread), (void*)this, 0, 0);
}

RenderThread::~RenderThread(){
	IF_DEBUG(Console::println(L"Destroying RenderThread"));
	this->send(make_shared<RTStopThreadMessage>());
	WaitForSingleObject(renderThread, INFINITE);
	CloseHandle(renderThread);
}


unsigned int WINAPI RenderThread::runRenderThread(void* lpParameter)
{
	reinterpret_cast<RenderThread*>(lpParameter)->threadProc();
	return 0;
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