#pragma once
// clang-format off
#include "Engine.fwd.h"
#include "DbAlbumInfo.h"
#include "TextDisplay.h"
#include "utils.h"
// clang-format on
namespace render {
using ::db::AlbumInfo;

class Renderer {
 public:
  explicit Renderer(engine::Engine& engine);
  void resizeGlScene(int width, int height);
  void setProjectionMatrix(bool pickMatrix = false, int x = 0, int y = 0);

  std::optional<AlbumInfo> albumAtPoint(int x, int y);

  void drawFrame();

  void ensureVSync(bool enableVSync);

  void glPushOrthoMatrix();
  void glPopOrthoMatrix();

  TextDisplay textDisplay;
  BitmapFont bitmapFont;
  engine::Engine& engine;

  bool wasMissingTextures = false;
  int winWidth = 1;
  int winHeight = 1;

 private:
  GLImage spinnerTexture;
  void getFrustrumSize(double& right, double& top, double& zNear, double& zFar);

  bool vSyncEnabled;

  void drawBg();
  void drawGui();
  void drawSpinner();
  void drawScene(bool selectionPass);

  pfc::array_t<double> getMirrorClipPlane();
  void drawMirrorPass();
  void drawMirrorOverlay();
  void drawCovers(bool showTarget = false);
};
} // namespace render
