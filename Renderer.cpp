// clang-format off
#include "Engine.h"    //(1)
#include "Renderer.h"  //(2)

#include "ConfigData.h"
#include "DbAlbumCollection.h"
#include "Image.h"
#include "TextureCache.h"
#include "cover_fovAspectBehaviour.h"
#include "lib/gl_structs.h"
#include "style_manager.h"
#include "utils.h"
#include "world_state.h"
// clang-format off
#define SELECTION_CENTER INT_MAX  // Selection is an unsigned int, so this is center
#define SELECTION_COVERS 1
#define SELECTION_MIRROR 2

namespace render {
using coverflow::configData;
using namespace engine;

using ::db::DBIter;

Renderer::Renderer(Engine& engine)
    : textDisplay(*this, engine.styleManager), bitmapFont(*this), engine(engine),
      spinnerTexture(loadSpinner()) {
  glfwSwapInterval(0);
  vSyncEnabled = false;
}

void Renderer::ensureVSync(bool enableVSync) {
  if (vSyncEnabled != enableVSync) {
    vSyncEnabled = enableVSync;
    glfwSwapInterval(enableVSync ? 1 : 0);
  }
}

void Renderer::resizeGlScene(int width, int height) {
  if (height == 0)
    height = 1;
  winWidth = width;
  winHeight = height;

  glViewport(0, 0, width, height);  // Reset The Current Viewport
  setProjectionMatrix();
  textDisplay.clearCache();
}

void Renderer::getFrustrumSize(double& right, double& top, double& zNear, double& zFar) {
  zNear = 0.1;
  zFar = 500;
  // Calculate The Aspect Ratio Of The Window
  static const double fov = 45;
  static const double squareLength = tan(deg2rad(fov) / 2) * zNear;
  fovAspectBehaviour weight = engine.coverPos.getAspectBehaviour();
  double aspect = double(winHeight) / winWidth;
  right = squareLength / pow(aspect, double(weight.y));
  top = squareLength * pow(aspect, double(weight.x));
}

void Renderer::setProjectionMatrix(bool pickMatrix, int x, int y) {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if (pickMatrix) {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, static_cast<GLint*>(viewport));
    gluPickMatrix(
        GLdouble(x), GLdouble(viewport[3] - y), 1.0, 1.0, static_cast<GLint*>(viewport));
  }
  double right, top, zNear, zFar;
  getFrustrumSize(right, top, zNear, zFar);
  glFrustum(-right, +right, -top, +top, zNear, zFar);
  glMatrixMode(GL_MODELVIEW);
}

void Renderer::glPushOrthoMatrix() {
  glDisable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, winWidth, 0, winHeight, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
}

void Renderer::glPopOrthoMatrix() {
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glEnable(GL_DEPTH_TEST);
}

std::optional<AlbumInfo> Renderer::albumAtPoint(int x, int y) {
  GLuint buf[256];
  glSelectBuffer(256, static_cast<GLuint*>(buf));
  glRenderMode(GL_SELECT);
  setProjectionMatrix(true, x, y);
  glInitNames();
  drawScene(true);
  GLint hits = glRenderMode(GL_RENDER);
  setProjectionMatrix();
  auto* p = static_cast<GLuint*>(buf);
  GLuint minZ = INFINITE;
  GLuint selectedName = 0;
  for (int i = 0; i < hits; i++) {
    GLuint names = *p;
    p++;
    GLuint z = *p;
    p++;
    p++;
    if ((names > 1) && (z < minZ) && (*p == SELECTION_COVERS)) {
      minZ = z;
      selectedName = *(p + 1);
    }
    p += names;
  }

  if (minZ == INFINITE)
    return std::nullopt;
  int offset = (selectedName - SELECTION_CENTER);
  auto& center = engine.worldState.getCenteredPos();
  if (auto iter = engine.db.iterFromPos(center)) {
    return engine.db.getAlbumInfo(engine.db.moveIterBy(iter.value(), offset));
  } else {
    return std::nullopt;
  }
}

void Renderer::drawMirrorPass() {
  glVectord mirrorNormal = engine.coverPos.getMirrorNormal();
  glVectord mirrorCenter = engine.coverPos.getMirrorCenter();

  double mirrorDist;  // distance from origin
  mirrorDist = mirrorCenter * mirrorNormal;
  glVectord mirrorPos = mirrorDist * mirrorNormal;

  glVectord scaleAxis(1, 0, 0);
  glVectord rotAxis = scaleAxis.cross(mirrorNormal);
  double rotAngle = rad2deg(scaleAxis.intersectAng(mirrorNormal));
  rotAngle = -2 * rotAngle;

  auto bgColor = engine.styleManager.getBgColorF();
  glFogfv(GL_FOG_COLOR,
          std::array<GLfloat, 4>{bgColor[0], bgColor[1], bgColor[2], 1.0f}.data());
  glEnable(GL_FOG);

  glPushMatrix();
  glTranslated(2 * mirrorPos.x, 2 * mirrorPos.y, 2 * mirrorPos.z);
  glScalef(-1.0f, 1.0f, 1.0f);
  glRotated(rotAngle, rotAxis.x, rotAxis.y, rotAxis.z);

  drawCovers();
  glPopMatrix();

  glDisable(GL_FOG);
}

void Renderer::drawMirrorOverlay() {
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  auto bgColor = engine.styleManager.getBgColorF();
  glColor4f(bgColor[0], bgColor[1], bgColor[2], 0.60f);

  glShadeModel(GL_FLAT);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glBegin(GL_QUADS);
  glVertex3i(-1, -1, -1);
  glVertex3i(1, -1, -1);
  glVertex3i(1, 1, -1);
  glVertex3i(-1, 1, -1);
  glEnd();
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_TEXTURE_2D);
  glShadeModel(GL_SMOOTH);
}

pfc::array_t<double> Renderer::getMirrorClipPlane() {
  glVectord mirrorNormal = engine.coverPos.getMirrorNormal();
  glVectord mirrorCenter = engine.coverPos.getMirrorCenter();
  glVectord eye2mCent = (mirrorCenter - engine.coverPos.getCameraPos());

  // Angle at which the mirror normal stands to the eyePos
  double mirrorNormalAngle = rad2deg(eye2mCent.intersectAng(mirrorNormal));
  pfc::array_t<double> clipEq;
  clipEq.set_size(4);
  if (mirrorNormalAngle > 90) {
    clipEq[0] = -mirrorNormal.x;
    clipEq[1] = -mirrorNormal.y;
    clipEq[2] = -mirrorNormal.z;
    clipEq[3] = mirrorNormal * mirrorCenter;
  } else {
    clipEq[0] = mirrorNormal.x;
    clipEq[1] = mirrorNormal.y;
    clipEq[2] = mirrorNormal.z;
    clipEq[3] = -(mirrorNormal * mirrorCenter);
  }
  return clipEq;
}

void Renderer::drawScene(bool selectionPass) {
  glLoadIdentity();
  auto cameraPos = engine.coverPos.getCameraPos();
  auto lookAt = engine.coverPos.getLookAt();
  auto up = engine.coverPos.getUpVector();
  gluLookAt(cameraPos.x, cameraPos.y, cameraPos.z, lookAt.x, lookAt.y, lookAt.z, up.x,
            up.y, up.z);

  pfc::array_t<double> clipEq;

  if (engine.coverPos.isMirrorPlaneEnabled()) {
    clipEq = getMirrorClipPlane();
    if (!selectionPass) {
      glClipPlane(GL_CLIP_PLANE0, clipEq.get_ptr());
      glEnable(GL_CLIP_PLANE0);
      glPushName(SELECTION_MIRROR);
      drawMirrorPass();
      glPopName();
      glDisable(GL_CLIP_PLANE0);
      drawMirrorOverlay();
    }

    // invert the clip equation
    for (int i = 0; i < 4; i++) {
      clipEq[i] *= -1;
    }

    glClipPlane(GL_CLIP_PLANE0, clipEq.get_ptr());
    glEnable(GL_CLIP_PLANE0);
  }

  glPushName(SELECTION_COVERS);
  drawCovers(true);
  glPopName();

  if (engine.coverPos.isMirrorPlaneEnabled()) {
    glDisable(GL_CLIP_PLANE0);
  }
}

void Renderer::drawGui() {
  if (configData->ShowAlbumTitle || engine.db.initializing()) {
    std::string albumTitle;
    std::vector<size_t> highlight;
    if (engine.db.initializing()) {
      albumTitle = "Generating Cover Display ...";
    } else if (engine.db.empty()) {
      albumTitle = "No Covers to Display";
    } else {
      if (engine.coverPos.isCoverTitleEnabled()) {
        DBIter iter = engine.db.iterFromPos(engine.worldState.getTarget()).value();
        albumTitle = engine.db.getAlbumInfo(iter).title;
        highlight = engine.findAsYouType.highlightPositions(albumTitle);
      }
    }
    textDisplay.displayText(albumTitle, highlight, int(winWidth * configData->TitlePosH),
                            int(winHeight * (1 - configData->TitlePosV)));
  }

  if (configData->ShowFps) {
    double avgDur, maxDur, avgCPU, maxCPU;
    engine.fpsCounter.getFPS(avgDur, maxDur, avgCPU, maxCPU);
    std::ostringstream dispStringA;
    std::ostringstream dispStringB;
    dispStringA.flags(std::ios_base::fixed | std::ios_base::right);
    dispStringB.flags(std::ios_base::fixed | std::ios_base::right);
    dispStringA.precision(1);
    dispStringB.precision(1);
    dispStringA << "FPS:       " << std::setw(4) << 1 / avgDur;
    dispStringB << "max ms/f: " << std::setw(5) << (1000 * maxDur);
    dispStringA << "  cpu: " << std::setw(5) << avgCPU * 1000;
    dispStringB << "  max: " << std::setw(5) << maxCPU * 1000;
    bitmapFont.displayText(dispStringA.str().c_str(), engine.styleManager.getTitleColor(),
                           15, winHeight - 20);
    bitmapFont.displayText(dispStringB.str().c_str(), engine.styleManager.getTitleColor(),
                           15, winHeight - 35);
  }

  if (engine.reloadWorker)
    drawSpinner();
}

void Renderer::drawSpinner() {
  glPushOrthoMatrix();

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  auto color = engine.styleManager.getTitleColorF();
  glColor3f(color[0], color[1], color[2]);
  spinnerTexture.bind();
  int x = winWidth - 30;
  int y = winHeight - 30;

  double rot = -time() * 10;
  double r = -sqrtf(2) * 16;
  glBegin(GL_QUADS);
  {
    glTexCoord2f(0.0f, 1.0f);  // top left
    glVertex3d(x + r * cos(rot + 0 * M_PI / 2), y + r * sin(rot + 0 * M_PI / 2), 0);
    glTexCoord2f(1.0f, 1.0f);  // top right
    glVertex3d(x + r * cos(rot + 1 * M_PI / 2), y + r * sin(rot + 1 * M_PI / 2), 0);
    glTexCoord2f(1.0f, 0.0f);  // bottom right
    glVertex3d(x + r * cos(rot + 2 * M_PI / 2), y + r * sin(rot + 2 * M_PI / 2), 0);
    glTexCoord2f(0.0f, 0.0f);  // bottom left
    glVertex3d(x + r * cos(rot + 3 * M_PI / 2), y + r * sin(rot + 3 * M_PI / 2), 0);
  }
  glEnd();
  glDisable(GL_BLEND);
  glPopOrthoMatrix();
}

void Renderer::drawBg() {
  auto bgColor = engine.styleManager.getBgColorF();
  glClearColor(bgColor[0], bgColor[1], bgColor[2], 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

bool PngAlphaEnabled(engine::Engine& engine) {
  return engine.coverPos.isCoverPngAlphaEnabled() && configData->CoverArtEnablePngAlpha;
}

void Renderer::drawFrame() {
  TRACK_CALL_TEXT("Renderer::drawFrame");
  wasMissingTextures = false;
  drawBg();
  drawScene(false);
  drawGui();
}
void Renderer::drawCovers(bool showTarget) {
  if (configData->HighlightWidth == 0)
    showTarget = false;

  if (engine.db.empty() && !engine.db.initializing())
    return;

  struct Cover {
    const GLImage* tex;
    float offset;
    int index;
    bool isTarget;
  };

  std::vector<Cover> covers;
  std::vector<std::pair<float, Cover>> covers_depth;

  if (engine.db.initializing()) {
    auto tex = &engine.texCache.getLoadingTexture();
    for (int i = engine.coverPos.getFirstCover(); i <= engine.coverPos.getLastCover();
         ++i) {
      if (!PngAlphaEnabled(engine))
        covers.push_back(Cover{tex, float(i), i, i == 0});
      else
        covers_depth.push_back({ float(i), Cover{tex, float(i), i, i == 0}  });
    }
  } else {
    float centerOffset = engine.worldState.getCenteredOffset();
    // We can assume that iterFromPos succeeds, because we already checked for empty db
    DBIter targetCover = engine.db.iterFromPos(engine.worldState.getTarget()).value();
    DBIter centerCover =
        engine.db.iterFromPos(engine.worldState.getCenteredPos()).value();
    DBIter firstCover =
        engine.db.moveIterBy(centerCover, engine.coverPos.getFirstCover() + 1);
    DBIter lastCover = engine.db.moveIterBy(centerCover, engine.coverPos.getLastCover());
    lastCover++;  // moveIterBy never returns the end() element

    int offset = engine.db.difference(firstCover, centerCover);

    for (DBIter p = firstCover; p != lastCover; ++p, ++offset) {
      float co = -centerOffset + offset;

      auto tex = engine.texCache.getAlbumTexture(p->key);
      if (tex == nullptr) {
        wasMissingTextures = true;
        tex = &engine.texCache.getLoadingTexture();
      }
      if (!PngAlphaEnabled(engine))
        covers.push_back(Cover{tex, co, offset, p == targetCover});
      else
        covers_depth.push_back({ co, Cover{tex, co, offset, p == targetCover} });
    }
  }
  if (PngAlphaEnabled(engine)) {
    //inverse sort and send back to covers
    std::sort(covers_depth.begin(), covers_depth.end(), [](auto& left, auto& right) {
      return abs(right.first) < abs(left.first);
      });
    std::transform(begin(covers_depth), end(covers_depth),
      std::back_inserter(covers),
      [](auto const& pair) { return pair.second; });
  }

  for (Cover cover : covers) {
    cover.tex->bind();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (PngAlphaEnabled(engine)) {
      //if (/*cover.tex->getAlpha() &&*/ cover.offset < 0.5) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      //}
    }

    // calculate darkening
    float g = 1 - (std::min(1.0f, (abs(cover.offset) - 2) / 5)) * 0.5f;
    if (abs(cover.offset) < 2)
      g = 1;
    glColor3f(g, g, g);

    glQuad coverQuad = engine.coverPos.getCoverQuad(cover.offset, cover.tex->getAspect());
    glPushName(SELECTION_CENTER + cover.index);
    glBegin(GL_QUADS);
    glFogCoordf(
        static_cast<GLfloat>(engine.coverPos.distanceToMirror(coverQuad.topLeft)));
    glTexCoord2f(0.0f, 0.0f);
    glVertex3fv(coverQuad.topLeft.as_3fv());

    glFogCoordf(
        static_cast<GLfloat>(engine.coverPos.distanceToMirror(coverQuad.topRight)));
    glTexCoord2f(1.0f, 0.0f);
    glVertex3fv(coverQuad.topRight.as_3fv());

    glFogCoordf(
        static_cast<GLfloat>(engine.coverPos.distanceToMirror(coverQuad.bottomRight)));
    glTexCoord2f(1.0f, 1.0f);
    glVertex3fv(coverQuad.bottomRight.as_3fv());

    glFogCoordf(
        static_cast<GLfloat>(engine.coverPos.distanceToMirror(coverQuad.bottomLeft)));
    glTexCoord2f(0.0f, 1.0f);
    glVertex3fv(coverQuad.bottomLeft.as_3fv());
    glEnd();
    glPopName();

    if (showTarget && cover.isTarget) {
      bool clipPlane = false;
      if (glIsEnabled(GL_CLIP_PLANE0)) {
        glDisable(GL_CLIP_PLANE0);
        clipPlane = true;
      }

      auto color = engine.styleManager.getTitleColorF();
      glColor3f(color[0], color[1], color[2]);

      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glDisable(GL_TEXTURE_2D);

      glLineWidth(GLfloat(configData->HighlightWidth));
      glPolygonOffset(-1.0f, -1.0f);
      glEnable(GL_POLYGON_OFFSET_LINE);

      glEnable(GL_VERTEX_ARRAY);
      glVertexPointer(3, GL_FLOAT, 0, static_cast<void*>(&coverQuad));
      glDrawArrays(GL_QUADS, 0, 4);

      glDisable(GL_POLYGON_OFFSET_LINE);

      glEnable(GL_TEXTURE_2D);

      if (clipPlane)
        glEnable(GL_CLIP_PLANE0);
    }
  }
}
}  // namespace render
