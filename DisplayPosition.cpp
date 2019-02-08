#include "DisplayPosition.h"

#include "Helpers.h"
#include "PlaybackTracer.h"
#include "base.h"
#include "config.h"

DisplayPosition::DisplayPosition(DbAlbumCollection& db)
    : db(db), centeredPos(db.begin()) {}

DisplayPosition::~DisplayPosition(void) {}

void DisplayPosition::onTargetChange() {
  if (!rendering) {
    lastMovement = Helpers::getHighresTimer();
    rendering = true;
  }
  CollectionPos targetPos = db.getTargetPos();
}

CollectionPos DisplayPosition::getOffsetPos(int n) const {
  CollectionPos p = centeredPos;
  db.movePosBy(p, n);
  return p;
}

void DisplayPosition::update(void) {
  double currentTime = Helpers::getHighresTimer();
  CollectionPos targetPos = db.getTargetPos();
  // do this here because of concurrency – isMoving might see a different targetPos
  if (centeredPos != targetPos || centeredOffset != 0) {
    int targetRank = db.rank(targetPos);
    int centeredRank = db.rank(centeredPos);
    float dist = (targetRank - centeredRank) - centeredOffset;
    float dTime = float(currentTime - lastMovement);
    float speed = abs(targetDist2moveDist(dist));
    if (lastSpeed < 0.1f)
      lastSpeed = 0.1f;
    float accel = (speed - lastSpeed) / dTime;
    if (accel > 0.0f) {
      accel = sqrt(accel);
      speed = lastSpeed + accel * dTime;
    }
    float moveDist = (dist > 0 ? 1.0f : -1.0f) * speed * dTime;

    if ((abs(moveDist) > abs(dist)) || abs(moveDist - dist) < 0.001f) {
      centeredPos = targetPos;
      centeredOffset = 0.0f;
      lastSpeed = 0.0f;
    } else {
      lastSpeed = speed;
      int moveDistInt = (int)floor(moveDist);

      float newOffset = centeredOffset;  // minimize MT risk
      newOffset += (moveDist - moveDistInt);
      if (newOffset >= 1.0f) {
        newOffset -= 1.0f;
        moveDistInt += 1;
      } else if (newOffset < 0.0f) {
        newOffset += 1.0f;
        moveDistInt -= 1;
      }
      centeredOffset = newOffset;
      std::advance(centeredPos, moveDistInt);
    }
  }
  if (!isMoving()) {
    rendering = false;
  }
  lastMovement = currentTime;
}

float DisplayPosition::targetDist2moveDist(float targetDist) {
  bool goRight = (targetDist > 0.0f);
  targetDist = abs(targetDist);
  return (goRight ? 1 : -1) *
         ((0.1f * targetDist * targetDist) + (0.9f * targetDist) + 2.0f);
}

bool DisplayPosition::isMoving(void) {
  return !((centeredPos == db.getTargetPos()) && (centeredOffset == 0));
}

CollectionPos DisplayPosition::getCenteredPos(void) const {
  return centeredPos;
}

float DisplayPosition::getCenteredOffset(void) const {
  return centeredOffset;
}

void DisplayPosition::hardSetCenteredPos(CollectionPos pos) {
  centeredPos = pos;
}
