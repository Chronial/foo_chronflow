// clang-format off
#include "world_state.h"
#include "DbAlbumCollection.h"
#include "PlaybackTracer.h"
#include "config.h"
#include "utils.h"

WorldState::WorldState(DbAlbumCollection& db) : db(db) {
  pfc::string8 selected;
  sessionSelectedCover.get(selected);
  targetPos.key = selected.c_str();
  centeredPos = targetPos;
}

const DBPos& WorldState::getTarget() {
  return targetPos;
}

void WorldState::setTarget(DBPos target) {
  targetPos = target;
  if (!rendering) {
    lastMovement = time() - 0.02;
    rendering = true;
  }
  sessionSelectedCover.set(target.key.c_str());
}

void WorldState::update() {
  if (db.empty())
    return;
  double currentTime = time();
  if (isMoving()) {
    float dist = db.difference(db.iterFromPos(targetPos).value(),
                               db.iterFromPos(centeredPos).value()) -
                 centeredOffset;
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
      int moveSteps = int(floor(moveDist));

      centeredOffset += (moveDist - moveSteps);
      if (centeredOffset >= 1.0f) {
        centeredOffset -= 1.0f;
        moveSteps += 1;
      } else if (centeredOffset < 0.0f) {
        centeredOffset += 1.0f;
        moveSteps -= 1;
      }
      centeredPos = db.movePosBy(centeredPos, moveSteps);
    }
  }
  if (!isMoving()) {
    rendering = false;
  }
  lastMovement = currentTime;
}

float WorldState::targetDist2moveDist(float targetDist) {
  bool goRight = (targetDist > 0.0f);
  targetDist = abs(targetDist);
  return (goRight ? 1 : -1) *
         ((0.1f * targetDist * targetDist) + (0.9f * targetDist) + 2.0f);
}

bool WorldState::isMoving() {
  return (centeredPos != targetPos) || (centeredOffset != 0);
}

const DBPos& WorldState::getCenteredPos() const {
  return centeredPos;
}

float WorldState::getCenteredOffset() const {
  return centeredOffset;
}

void WorldState::hardSetCenteredPos(DBPos pos) {
  centeredPos = std::move(pos);
}
} // namespace worldstate
