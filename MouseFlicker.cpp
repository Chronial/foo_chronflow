#include "chronflow.h"

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
		double pD = win2pos(dX);
		float dist = appInstance->displayPos->moveDist2targetDist(float(pD / dTime));
		dist += appInstance->displayPos->getCenteredOffset();
		appInstance->displayPos->setTarget(appInstance->displayPos->getCenteredPos() + (int)floor(dist+0.5));
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
