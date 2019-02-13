#include "DisplayPosition.h"

#include "PlaybackTracer.h"
#include "config.h"
#include "utils.h"

DisplayPosition::DisplayPosition(DbAlbumCollection& db)
    : db(db), centeredPos(db.begin()) {}

void DisplayPosition::onTargetChange() {
  if (!rendering) {
    lastMovement = Helpers::getHighresTimer() - 0.02;
    rendering = true;
  }
}

CollectionPos DisplayPosition::getOffsetPos(int n) const {
  CollectionPos p = centeredPos;
  db.movePosBy(p, n);
  return p;
}

void DisplayPosition::update() {
  double currentTime = Helpers::getHighresTimer();
  CollectionPos targetPos = db.getTargetPos();
  if (isMoving()) {
    int targetRank = db.rank(targetPos);
    int centeredRank = db.rank(centeredPos);
    float dist = (targetRank - centeredRank) - centeredOffset;
    auto dTime = float(currentTime - lastMovement);
    float speed = abs(targetDist2moveDist(dist));
    if (lastSpeed < speed) {
      float accel = (speed - lastSpeed) / dTime;
      accel = sqrt(accel);
      speed = lastSpeed + accel * dTime;
    }
    speed = std::max(0.1f, speed);  // minimum speed
    float moveDist = (dist > 0 ? 1.0f : -1.0f) * speed * dTime;

    if (dTime > 1 || abs(moveDist) > abs(dist) || abs(moveDist - dist) < 0.001f) {
      centeredPos = targetPos;
      centeredOffset = 0.0f;
      lastSpeed = 0.0f;
    } else {
      lastSpeed = speed;
      float moveDistFloor = floor(moveDist);

      float newOffset = centeredOffset;  // minimize MT risk
      newOffset += (moveDist - moveDistFloor);
      if (newOffset >= 1.0f) {
        newOffset -= 1.0f;
        moveDistFloor += 1;
      } else if (newOffset < 0.0f) {
        newOffset += 1.0f;
        moveDistFloor -= 1;
      }
      centeredOffset = newOffset;
      std::advance(centeredPos, moveDistFloor);
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

bool DisplayPosition::isMoving() {
  return !((centeredPos == db.getTargetPos()) && (centeredOffset == 0));
}

CollectionPos DisplayPosition::getCenteredPos() const {
  return centeredPos;
}

float DisplayPosition::getCenteredOffset() const {
  return centeredOffset;
}

void DisplayPosition::hardSetCenteredPos(CollectionPos pos) {
  centeredPos = pos;
}
