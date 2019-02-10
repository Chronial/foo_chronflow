#pragma once
#include "DbAlbumCollection.h"

class DisplayPosition {
  DbAlbumCollection& db;

 public:
  explicit DisplayPosition(DbAlbumCollection& db);
  void update();

  bool isMoving();

  /// Position centered or left of center
  /// eg: display between 2. and 3. cover -> CenteredPos = 2. cover
  CollectionPos getCenteredPos() const;

  /// return in range [0;1)  (eg. display on pos 2.59 -- returns 0.59)u
  float getCenteredOffset() const;

  /// Relative to centeredPos
  CollectionPos getOffsetPos(int n) const;

  void hardSetCenteredPos(CollectionPos pos);

  void onTargetChange();

 private:
  float targetDist2moveDist(float targetDist);

  bool rendering = false;

  CollectionPos centeredPos;
  volatile float centeredOffset = 0.0f;
  float lastSpeed = 0.0f;
  double lastMovement = 0.0;
};
