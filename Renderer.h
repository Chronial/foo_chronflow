#pragma once
#include "DbAlbumCollection.h"
#include "TextDisplay.h"
#include "utils.h"

class TextureCache;
class WorldState;
class Engine;

class Renderer {
 public:
  explicit Renderer(Engine& engine);

  void resizeGlScene(int width, int height);
  void setProjectionMatrix(bool pickMatrix = false, int x = 0, int y = 0);

  std::optional<AlbumInfo> albumAtPoint(int x, int y);

  void drawFrame();

  void ensureVSync(bool enableVSync);

  void glPushOrthoMatrix();
  void glPopOrthoMatrix();

  TextDisplay textDisplay;
  class Engine& engine;

  bool wasMissingTextures = false;

 private:
  void getFrustrumSize(double& right, double& top, double& zNear, double& zFar);

  int winWidth = 1;
  int winHeight = 1;

  bool vSyncEnabled;

  void drawBg();
  void drawGui();
  void drawScene(bool selectionPass);

  pfc::array_t<double> getMirrorClipPlane();
  void drawMirrorPass();
  void drawMirrorOverlay();
  void drawCovers(bool showTarget = false);
};
