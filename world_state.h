#pragma once
#include "DbAlbumCollection.h"

class WorldState {
  DbAlbumCollection& db;

 public:
  explicit WorldState(DbAlbumCollection& db);
  void update();
  bool isMoving();

  const DBPos& getCenteredPos() const;
  void hardSetCenteredPos(DBPos pos);

  /// return in range [0;1)  (eg. display on pos 2.59 -- returns 0.59)u
  float getCenteredOffset() const;

  const DBPos& getTarget();
  void setTarget(DBPos target);

 private:
  float targetDist2moveDist(float targetDist);

  bool rendering = false;

  DBPos centeredPos;
  DBPos targetPos;
  volatile float centeredOffset = 0.0f;
  float lastSpeed = 0.0f;
  double lastMovement = 0.0;
};
