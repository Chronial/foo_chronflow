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
	  targetPos(startingPos),
	  lastSpeed(0.0f)
{
}


DisplayPosition::~DisplayPosition(void)
{
}

void DisplayPosition::setTarget(CollectionPos pos)
{
	ScopeCS scopeLock(accessCS);
	if (!isMoving()){
		lastMovement = Helpers::getHighresTimer();
		if (targetPos != pos)
			appInstance->redrawMainWin();
	}
	targetPos = pos;
	
	sessionSelectedCover = appInstance->albumCollection->rank(pos);
}

void DisplayPosition::moveTargetBy(int n)
{
	ScopeCS scopeLock(accessCS);
	CollectionPos p = targetPos;
	moveIteratorBy(p, n);
	setTarget(p);
}

CollectionPos DisplayPosition::getOffsetPos(int n) const
{
	CollectionPos p = centeredPos;
	moveIteratorBy(p, n);
	return p;
}

inline void DisplayPosition::moveIteratorBy(CollectionPos& p, int n) const
{
	if (n > 0){
		CollectionPos next;
		for (int i = 0; i < n; ++i){
			if (p == appInstance->albumCollection->end())
				break;
			next = p;
			++next;
			// Do only move just before the end, don't reach the end
			if (next == appInstance->albumCollection->end())
				break;
			p = next;
		}
	} else {
		for (int i = 0; i > n && p != appInstance->albumCollection->begin(); --p, --i){}
	}
}


void DisplayPosition::update(void)
{
	CS_ASSERT(accessCS);
	double currentTime = Helpers::getHighresTimer();
	if (isMoving()){
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
	return !((centeredPos == targetPos) && (centeredOffset == 0));
}

CollectionPos DisplayPosition::getTarget(void) const
{
	return targetPos;
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
	CS_ASSERT(accessCS);
	centeredPos = pos;
}

void DisplayPosition::hardSetTarget(CollectionPos pos)
{
	CS_ASSERT(accessCS);
	targetPos = pos;
}