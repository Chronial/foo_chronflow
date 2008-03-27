#include "chronflow.h"

extern cfg_int sessionSelectedCover;

DisplayPosition::DisplayPosition(AppInstance* instance, CollectionPos startingPos)
	: centeredOffset(0.0f),
	  appInstance(instance),
	  centeredPos(startingPos),
	  targetPos(startingPos),
	  lastSpeed(0.0f)
{
}


DisplayPosition::~DisplayPosition(void)
{
}

void DisplayPosition::setTarget(CollectionPos pos)
{
	if (!isMoving()){
		lastMovement = Helpers::getHighresTimer();
		RedrawWindow(appInstance->mainWindow,NULL,NULL,RDW_INVALIDATE);
	}
	targetPos = pos;
	sessionSelectedCover = pos.toIndex();
}

void DisplayPosition::update(void)
{
	double currentTime = Helpers::getHighresTimer();
	if (isMoving()){
		float dist = (targetPos - centeredPos) - centeredOffset;
		float dTime = float(currentTime - lastMovement);
		float speed = abs(targetDist2moveDist(dist));
		if (lastSpeed < 0.1f)
			lastSpeed = 0.1f;
		float accel = (speed - lastSpeed) / dTime;
		if (accel > 0.0f){
			accel = sqrt(accel);
			speed = lastSpeed + accel*dTime;
		}
		float moveDist = (dist>0 ? 1.0f : -1.0f) * speed * dTime;

		if ((abs(moveDist) > abs(dist)) || abs(moveDist - dist) < 0.001f){
			centeredPos = targetPos;
			centeredOffset = 0.0f;
			lastSpeed = 0.0f;
		} else {
			lastSpeed = speed;
			int moveDistInt = (int)floor(moveDist);
			centeredOffset += (moveDist - moveDistInt);
			if (centeredOffset >= 1.0f){
				centeredOffset -= 1.0f;
				moveDistInt += 1;
			} else if (centeredOffset < 0.0f){
				centeredOffset += 1.0f;
				moveDistInt -= 1;
			}
			centeredPos += moveDistInt;
		}
	}
	if (!isMoving()){
		appInstance->playbackTracer->movementEnded();
	}
	lastMovement = currentTime;
}

float DisplayPosition::targetDist2moveDist(float targetDist){
	bool goRight = (targetDist > 0.0f);
	targetDist = abs(targetDist);
	return (goRight?1:-1) * ((0.1f * targetDist*targetDist) + (0.9f * targetDist) + 2.0f);
}

float DisplayPosition::moveDist2targetDist(float moveDist){
	bool goRight = (moveDist > 0.0f);
	moveDist = abs(moveDist);
	if (moveDist < 2){
		return 0;
	} else {
		return float((goRight?1:-1) * (-(9.0/2) + sqrt((81.0/4) - 20 + 10*moveDist)));
	}
}

bool DisplayPosition::isMoving(void)
{
	return !((centeredPos == targetPos) && (centeredOffset == 0));
}

CollectionPos DisplayPosition::getTarget(void)
{
	return targetPos;
}

CollectionPos DisplayPosition::getCenteredPos(void)
{
	return centeredPos;
}

float DisplayPosition::getCenteredOffset(void)
{
	return centeredOffset;
}

void DisplayPosition::hardSetCenteredPos(CollectionPos pos)
{
	centeredPos = pos;
}

void DisplayPosition::hardSetTarget(CollectionPos pos)
{
	targetPos = pos;
}