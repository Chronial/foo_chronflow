#pragma once
#include "Helpers.h"

#include "DbAlbumCollection.h"
#include "TextDisplay.h"
#include "cover_positions.h"

class TextureCache;
class DisplayPosition;
class Engine;

enum VSyncMode {
  VSYNC_SLEEP_ONLY = 1,
  VSYNC_AND_SLEEP = 2,
  VSYNC_ONLY = 3,
};

class Renderer {
 public:
  Renderer(Engine& engine);

  void resizeGlScene(int width, int height);
  void setProjectionMatrix(bool pickMatrix = false, int x = 0, int y = 0);

  bool offsetOnPoint(int x, int y, int& out);

  void drawFrame();

  void ensureVSync(bool enableVSync);

  void glPushOrthoMatrix();
  void glPopOrthoMatrix();

  TextDisplay textDisplay;
  FpsCounter fpsCounter;
  ScriptedCoverPositions coverPos;
  class Engine& engine;

 private:
  void getFrustrumSize(double& right, double& top, double& zNear, double& zFar);

  int winWidth;
  int winHeight;

  bool vSyncEnabled;

  void drawBg();
  void drawGui();
  void drawScene(bool selectionPass);

  pfc::array_t<double> getMirrorClipPlane();
  void drawMirrorPass();
  void drawMirrorOverlay();
  void drawCovers(bool showTarget = false);
};
