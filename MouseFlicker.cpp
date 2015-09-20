#include "stdafx.h"
#include "base.h"

#include "AppInstance.h"
#include "DisplayPosition.h"
#include "Helpers.h"
#include "MouseFlicker.h"
#include "PlaybackTracer.h"

MouseFlicker::MouseFlicker(AppInstance* instance) :
appInstance(instance),
isMouseDown(false)
{
}

MouseFlicker::~MouseFlicker(void)
{
	ReleaseCapture();
}

void MouseFlicker::mouseDown(HWND hWnd, int x, int y)
{
	isMouseDown = true;
	lastX = x;
	lastTime = 0;
	dX = 0;
	dTime = 1;
	SetCapture(hWnd);
	appInstance->playbackTracer->userStartedMovement(); // maybe we should implement a PlaybackTracer::MouseFlickerDown
}

void MouseFlicker::mouseUp(int x, int y)
{
	if (isMouseDown){
		ScopeCS scopeLock(appInstance->displayPos->accessCS);
		double pD = win2pos(dX);
		float dist = appInstance->displayPos->moveDist2targetDist(float(pD / dTime));
		if (abs(dist) > 0.5){
			dist += appInstance->displayPos->getCenteredOffset();
			dist *= -1;
			appInstance->displayPos->moveTargetBy((int)floor(dist+0.5));
		}
		isMouseDown = false;
	}
	ReleaseCapture();
}

void MouseFlicker::mouseMove(int x, int y)
{
	if (isMouseDown){
		double time = Helpers::getHighresTimer();

		dX = x - lastX;
		dTime = time - lastTime;
		lastX = x;
		lastTime = time;

		/*double pD = win2pos(dX);
		pD += dPos->getCenteredOffset();
		int pDi = int(floor(pD));
		float pDo = float(pD - pDi);
		dPos->setCenter(dPos->getCenteredPos() + pDi, pDo);*/
	}
}

double MouseFlicker::win2pos(int x)
{
	return float(x)/70;
}


void MouseFlicker::lostCapture(int x, int y)
{
	if (isMouseDown){
		appInstance->playbackTracer->movementEnded(); // Hack, but we shouldn't loose capture after all
	}
	isMouseDown = false;
}
