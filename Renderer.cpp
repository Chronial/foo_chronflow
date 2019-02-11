#include "Renderer.h"

#include "lib/gl_structs.h"

#include "DbAlbumCollection.h"
#include "DisplayPosition.h"
#include "Engine.h"
#include "Helpers.h"
#include "TextureCache.h"
#include "config.h"

#define SELECTION_CENTER INT_MAX  // Selection is an unsigned int, so this is center
#define SELECTION_COVERS 1
#define SELECTION_MIRROR 2

Renderer::Renderer(Engine& engine) : textDisplay(this), engine(engine) {
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
}

void Renderer::getFrustrumSize(double& right, double& top, double& zNear, double& zFar) {
  zNear = 0.1;
  zFar = 500;
  // Calculate The Aspect Ratio Of The Window
  static const double fov = 45;
  static const double squareLength = tan(deg2rad(fov) / 2) * zNear;
  fovAspectBehaviour weight = coverPos.getAspectBehaviour();
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

bool Renderer::offsetOnPoint(int x, int y, int& out) {
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
  if (minZ != INFINITE) {
    out = (selectedName - SELECTION_CENTER);
    return true;
  } else {
    return false;
  }
}

void Renderer::drawMirrorPass() {
  glVectord mirrorNormal = coverPos.getMirrorNormal();
  glVectord mirrorCenter = coverPos.getMirrorCenter();

  double mirrorDist;  // distance from origin
  mirrorDist = mirrorCenter * mirrorNormal;
  glVectord mirrorPos = mirrorDist * mirrorNormal;

  glVectord scaleAxis(1, 0, 0);
  glVectord rotAxis = scaleAxis.cross(mirrorNormal);
  double rotAngle = rad2deg(scaleAxis.intersectAng(mirrorNormal));
  rotAngle = -2 * rotAngle;

  glFogfv(GL_FOG_COLOR, std::array<GLfloat, 4>{GetRValue(cfgPanelBg) / 255.0f,
                                               GetGValue(cfgPanelBg) / 255.0f,
                                               GetBValue(cfgPanelBg) / 255.0f, 1.0f}
                            .data());
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
  glColor4f(GetRValue(cfgPanelBg) / 255.0f, GetGValue(cfgPanelBg) / 255.0f,
            GetBValue(cfgPanelBg) / 255.0f, 0.60f);

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
  glVectord mirrorNormal = coverPos.getMirrorNormal();
  glVectord mirrorCenter = coverPos.getMirrorCenter();
  glVectord eye2mCent = (mirrorCenter - coverPos.getCameraPos());

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
  gluLookAt(coverPos.getCameraPos().x, coverPos.getCameraPos().y,
            coverPos.getCameraPos().z, coverPos.getLookAt().x, coverPos.getLookAt().y,
            coverPos.getLookAt().z, coverPos.getUpVector().x, coverPos.getUpVector().y,
            coverPos.getUpVector().z);

  pfc::array_t<double> clipEq;

  if (coverPos.isMirrorPlaneEnabled()) {
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

  if (coverPos.isMirrorPlaneEnabled()) {
    glDisable(GL_CLIP_PLANE0);
  }
}

void Renderer::drawGui() {
  if (cfgShowAlbumTitle || engine.db.getCount() == 0) {
    pfc::string8 albumTitle;
    if (engine.db.getCount()) {
      engine.db.getTitle(engine.db.getTargetPos(), albumTitle);
    } else if (engine.reloadWorker) {
      albumTitle = "Generating Cover Display ...";
    } else {
      albumTitle = "No Covers to Display";
    }
    textDisplay.displayText(albumTitle, int(winWidth * cfgTitlePosH),
                            int(winHeight * (1 - cfgTitlePosV)), TextDisplay::center,
                            TextDisplay::middle);
  }

  if (cfgShowFps) {
    double fps, msPerFrame, longestFrame, minFPS;
    fpsCounter.getFPS(fps, msPerFrame, longestFrame, minFPS);
    pfc::string8 dispStringA;
    pfc::string8 dispStringB;
    dispStringA << "FPS:  " << pfc::format_float(fps, 4, 1);
    dispStringB << " min: " << pfc::format_float(minFPS, 4, 1);
    dispStringA << "  cpu-ms/f: " << pfc::format_float(msPerFrame, 5, 2);
    dispStringB << "   max:     " << pfc::format_float(longestFrame, 5, 2);
    textDisplay.displayBitmapText(dispStringA, winWidth - 250, winHeight - 20);
    textDisplay.displayBitmapText(dispStringB, winWidth - 250, winHeight - 35);
  }
}

void Renderer::drawBg() {
  glClearColor(GetRValue(cfgPanelBg) / 255.0f, GetGValue(cfgPanelBg) / 255.0f,
               GetBValue(cfgPanelBg) / 255.0f, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::drawFrame() {
  TRACK_CALL_TEXT("Renderer::drawFrame");
  drawBg();
  drawScene(false);
  drawGui();
}

void Renderer::drawCovers(bool showTarget) {
  if (cfgHighlightWidth == 0)
    showTarget = false;

  if (engine.db.getCount() == 0)
    return;

  float centerOffset = engine.displayPos.getCenteredOffset();
  CollectionPos centerCover = engine.displayPos.getCenteredPos();
  CollectionPos firstCover = engine.displayPos.getOffsetPos(coverPos.getFirstCover() + 1);
  CollectionPos lastCover = engine.displayPos.getOffsetPos(coverPos.getLastCover());
  lastCover++;  // getOffsetPos does not return the end() element
  CollectionPos targetCover = engine.db.getTargetPos();

  int offset = engine.db.rank(firstCover) - engine.db.rank(centerCover);

  for (CollectionPos p = firstCover; p != lastCover; ++p, ++offset) {
    float co = -centerOffset + offset;

    const GLTexture& tex = engine.texCache.getLoadedImgTexture(p->groupString);
    tex.bind();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // calculate darkening
    float g = 1 - (std::min(1.0f, (abs(co) - 2) / 5)) * 0.5f;
    if (abs(co) < 2)
      g = 1;
    /*float g = 1 - (abs(co)-2)*0.2f;
    g = 1 - abs(co)*0.1f;
    g = 1 - abs(zRot)/80;
    g= 1;
    if (g < 0) g = 0;*/
    glColor3f(g, g, g);
    glVectord origin(0, 0.5, 0);

    glQuad coverQuad = coverPos.getCoverQuad(co, tex.getAspect());

    glPushName(SELECTION_CENTER + offset);

    glBegin(GL_QUADS);
    glFogCoordf(static_cast<GLfloat>(coverPos.distanceToMirror(coverQuad.topLeft)));
    glTexCoord2f(0.0f, 1.0f);  // top left
    glVertex3fv(coverQuad.topLeft.as_3fv());

    glFogCoordf(static_cast<GLfloat>(coverPos.distanceToMirror(coverQuad.topRight)));
    glTexCoord2f(1.0f, 1.0f);  // top right
    glVertex3fv(coverQuad.topRight.as_3fv());

    glFogCoordf(static_cast<GLfloat>(coverPos.distanceToMirror(coverQuad.bottomRight)));
    glTexCoord2f(1.0f, 0.0f);  // bottom right
    glVertex3fv(coverQuad.bottomRight.as_3fv());

    glFogCoordf(static_cast<GLfloat>(coverPos.distanceToMirror(coverQuad.bottomLeft)));
    glTexCoord2f(0.0f, 0.0f);  // bottom left
    glVertex3fv(coverQuad.bottomLeft.as_3fv());
    glEnd();
    glPopName();

    if (showTarget) {
      if (p == targetCover) {
        bool clipPlane = false;
        if (glIsEnabled(GL_CLIP_PLANE0)) {
          glDisable(GL_CLIP_PLANE0);
          clipPlane = true;
        }

        showTarget = false;

        glColor3f(GetRValue(cfgTitleColor) / 255.0f, GetGValue(cfgTitleColor) / 255.0f,
                  GetBValue(cfgTitleColor) / 255.0f);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_TEXTURE_2D);

        glLineWidth(GLfloat(cfgHighlightWidth));
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
}
