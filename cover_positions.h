#pragma once
#include "lib/gl_structs.h"
#include "cover_positions_compiler.h"

class ScriptedCoverPositions {
public:
  ScriptedCoverPositions(shared_ptr<CompiledCPInfo> cinfo)
    : cInfo(std::move(cinfo)) {
  };

  const fovAspectBehaviour& getAspectBehaviour();
  const glVectord& getLookAt();
  const glVectord& getUpVector();
  bool isMirrorPlaneEnabled();
  const glVectord& getMirrorNormal();
  const glVectord& getMirrorCenter();
  const glVectord& getCameraPos();
  const int getFirstCover();
  const int getLastCover();
  double distanceToMirror(glVectord point);
  glQuad getCoverQuad(float coverId, float coverAspect);

  bool isCoverTitleEnabled();
  bool isCoverPngAlphaEnabled();

private:
  shared_ptr<CompiledCPInfo> cInfo;
};
