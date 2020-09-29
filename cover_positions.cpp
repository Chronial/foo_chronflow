#include "cover_positions.h"

const fovAspectBehaviour& ScriptedCoverPositions::getAspectBehaviour() {
  return cInfo->aspectBehaviour;
}

const glVectord& ScriptedCoverPositions::getLookAt() { return cInfo->lookAt; }

const glVectord& ScriptedCoverPositions::getUpVector() { return cInfo->upVector; }

bool ScriptedCoverPositions::isMirrorPlaneEnabled() { return cInfo->showMirrorPlane; }

const glVectord& ScriptedCoverPositions::getMirrorNormal() { return cInfo->mirrorNormal; }

const glVectord& ScriptedCoverPositions::getMirrorCenter() { return cInfo->mirrorCenter; }

const glVectord& ScriptedCoverPositions::getCameraPos() { return cInfo->cameraPos; }

const int ScriptedCoverPositions::getFirstCover() { return cInfo->firstCover; }

const int ScriptedCoverPositions::getLastCover() { return cInfo->lastCover; }

double ScriptedCoverPositions::distanceToMirror(glVectord point) {
  return abs((cInfo->mirrorCenter - point) * cInfo->mirrorNormal);
}

glQuad ScriptedCoverPositions::getCoverQuad(float coverId, float coverAspect) {
  CoverPosInfo cPos = cInfo->getCoverPosInfo(coverId);

  double sizeLimAspect = cPos.sizeLim.w / cPos.sizeLim.h;

  float w;
  float h;
  if (coverAspect > sizeLimAspect) {
    w = cPos.sizeLim.w;
    h = w / coverAspect;
  } else {
    h = cPos.sizeLim.h;
    w = h * coverAspect;
  }

  glQuad out{};

  // out.topLeft.x = -w/2 -align.x * w/2;
  out.topLeft.x = (-1 - cPos.alignment.x) * w / 2;
  out.bottomLeft.x = out.topLeft.x;
  out.topRight.x = (1 - cPos.alignment.x) * w / 2;
  out.bottomRight.x = out.topRight.x;

  out.topLeft.y = (1 - cPos.alignment.y) * h / 2;
  out.topRight.y = out.topLeft.y;
  out.bottomLeft.y = (-1 - cPos.alignment.y) * h / 2;
  out.bottomRight.y = out.bottomLeft.y;

  out.topLeft.z = 0;
  out.topRight.z = 0;
  out.bottomLeft.z = 0;
  out.bottomRight.z = 0;

  out.rotate(cPos.rotation.a, cPos.rotation.axis);

  out.topLeft = cPos.position + out.topLeft;
  out.topRight = cPos.position + out.topRight;
  out.bottomLeft = cPos.position + out.bottomLeft;
  out.bottomRight = cPos.position + out.bottomRight;

  return out;
}

bool ScriptedCoverPositions::isCoverTitleEnabled() {
  return cInfo->enableCoverTitle;
}

bool ScriptedCoverPositions::isCoverPngAlphaEnabled() {
  return cInfo->enableCoverPngAlpha;
}
