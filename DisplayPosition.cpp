#include "stdafx.h"
#include "base.h"
#include "config.h"
#include "Helpers.h"

#include "DisplayPosition.h"

#include "AppInstance.h"
#include "PlaybackTracer.h"


DisplayPosition::DisplayPosition(AppInstance* instance, CollectionPos startingPos)
	: centeredOffset(0.0f),
	  appInstance(instance),
	  centeredPos(startingPos),
	  lastSpeed(0.0f),
	  rendering(false)
{
}


DisplayPosition::~DisplayPosition(void)
{
}

void DisplayPosition::onTargetChange()
{
	ASSERT_SHARED(appInstance->albumCollection);
	if (!rendering){
		lastMovement = Helpers::getHighresTimer();
		rendering = true;
	}
	CollectionPos targetPos = appInstance->albumCollection->getTargetPos();
	appInstance->redrawMainWin();
}


CollectionPos DisplayPosition::getOffsetPos(int n) const
{
	ASSERT_SHARED(appInstance->albumCollection);
	CollectionPos p = centeredPos;
	appInstance->albumCollection->movePosBy(p, n);
	return p;
}

void DisplayPosition::update(void)
{
	double currentTime = Helpers::getHighresTimer();
	CollectionPos targetPos = appInstance->albumCollection->getTargetPos();
	// do this here because of concurrency – isMoving might see a different targetPos
	if (centeredPos != targetPos || centeredOffset != 0){
		int targetRank = appInstance->albumCollection->rank(targetPos);
		int centeredRank = appInstance->albumCollection->rank(centeredPos);
		float dist = (targetRank - centeredRank) - centeredOffset;
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

			float newOffset = centeredOffset; // minimize MT risk
			newOffset += (moveDist - moveDistInt);
			if (newOffset >= 1.0f){
				newOffset -= 1.0f;
				moveDistInt += 1;
			} else if (newOffset < 0.0f){
				newOffset += 1.0f;
				moveDistInt -= 1;
			}
			centeredOffset = newOffset;
			std::advance(centeredPos, moveDistInt);
		}
	}
	if (!isMoving()){
		rendering = false;
		appInstance->playbackTracer->movementEnded();
	}
	lastMovement = currentTime;
}

float DisplayPosition::targetDist2moveDist(float targetDist){
	bool goRight = (targetDist > 0.0f);
	targetDist = abs(targetDist);
	return (goRight?1:-1) * ((0.1f * targetDist*targetDist) + (0.9f * targetDist) + 2.0f);
}

bool DisplayPosition::isMoving(void)
{
	return !((centeredPos == appInstance->albumCollection->getTargetPos()) && (centeredOffset == 0));
}

CollectionPos DisplayPosition::getCenteredPos(void) const
{
	return centeredPos;
}

float DisplayPosition::getCenteredOffset(void) const
{
	return centeredOffset;
}

void DisplayPosition::hardSetCenteredPos(CollectionPos pos)
{
	centeredPos = pos;
}