#include "Engine.h"

#include "Console.h"
#include "DisplayPosition.h"
#include "EngineWindow.h"
#include "MyActions.h"
#include "PlaybackTracer.h"
#include "TextureCache.h"
#include "config.h"

GLContext::GLContext(EngineWindow& window) {
  window.makeContextCurrent();
  // Static, since we only want to initialize the extensions once.
  // This also takes care of multithread synchronization.
  static const bool glad = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  if (!(glad && GLAD_GL_VERSION_2_1 && GLAD_GL_EXT_texture_filter_anisotropic &&
        GLAD_GL_EXT_bgra)) {
    throw std::exception("Glad failed to initialize OpenGl");
  }

  glShadeModel(GL_SMOOTH);  // Enable Smooth Shading
  glClearDepth(1.0f);       // Depth Buffer Setup
  glEnable(GL_DEPTH_TEST);  // Enables Depth Testing
  glDepthFunc(GL_LEQUAL);   // The Type Of Depth Testing To Do
  glHint(GL_PERSPECTIVE_CORRECTION_HINT,
         GL_NICEST);  // Really Nice Perspective Calculations
  glHint(GL_TEXTURE_COMPRESSION_HINT, GL_FASTEST);
  glEnable(GL_TEXTURE_2D);

  glFogi(GL_FOG_MODE, GL_EXP);
  glFogf(GL_FOG_DENSITY, 5);
  GLfloat fogColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};  // Fog Color - should be BG color
  glFogfv(GL_FOG_COLOR, fogColor);                 // Set The Fog Color
  glHint(GL_FOG_HINT, GL_NICEST);                  // Per-Pixel Fog Calculation
  glFogi(GL_FOG_COORD_SRC, GL_FOG_COORD);          // Set Fog Based On Vertice Coordinates
}

Engine::Engine(EngineThread& thread, EngineWindow& window)
    : thread(thread), window(window), glContext(window), db(), findAsYouType(*this),
      displayPos(db), playbackTracer(thread), texCache(thread, db), renderer(*this),
      afterLastSwap(0) {
  TIMECAPS tc;
  if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) == TIMERR_NOERROR) {
    timerResolution = std::min(std::max(tc.wPeriodMin, (UINT)1), tc.wPeriodMax);
  }
}

void Engine::mainLoop() {
  updateRefreshRate();
  EM::ReloadCollection().execute(*this);

  for (;;) {
    if (thread.messageQueue.size() == 0 && doPaint) {
      texCache.uploadTextures();
      texCache.trimCache();
      onPaint();
      continue;
    }
    std::unique_ptr<engine_messages::Message> msg = thread.messageQueue.pop();

    if (dynamic_cast<EM::StopThreadMessage*>(msg.get())) {
      break;
    } else if (dynamic_cast<EM::PaintMessage*>(msg.get())) {
      // do nothing, this is just here so that onPaint may run
    } else {
      msg->execute(*this);
    }
  }
  glFinish();
}

void Engine::onPaint() {
  TRACK_CALL_TEXT("EngineThread::onPaint");
  double frameStart = Helpers::getHighresTimer();

  displayPos.update();
  // continue animation if we are not done
  doPaint = displayPos.isMoving();
  // doPaint = true;

  renderer.drawFrame();

  // this might not be right – do we need a glFinish() here?
  double frameEnd = Helpers::getHighresTimer();
  renderer.fpsCounter.recordFrame(frameStart, frameEnd);

  renderer.ensureVSync(cfgVSyncMode != VSYNC_SLEEP_ONLY);
  if (cfgVSyncMode == VSYNC_AND_SLEEP || cfgVSyncMode == VSYNC_SLEEP_ONLY) {
    double currentTime = Helpers::getHighresTimer();
    // time we have        time we already have spend
    int sleepTime =
        static_cast<int>((1000.0 / refreshRate) - 1000 * (currentTime - afterLastSwap));
    if (cfgVSyncMode == VSYNC_AND_SLEEP)
      sleepTime -= 2 * timerResolution;
    else
      sleepTime -= timerResolution;
    if (sleepTime >= 0) {
      if (!timerInPeriod) {
        timeBeginPeriod(timerResolution);
        timerInPeriod = true;
      }
      Sleep(sleepTime);
    }
  }

  window.swapBuffers();
  afterLastSwap = Helpers::getHighresTimer();

  if (doPaint) {
    this->thread.send<EM::PaintMessage>();
  } else {
    if (timerInPeriod) {
      timeEndPeriod(timerResolution);
      timerInPeriod = false;
    }
  }
}

void Engine::updateRefreshRate() {
  DEVMODE dispSettings;
  ZeroMemory(&dispSettings, sizeof(dispSettings));
  dispSettings.dmSize = sizeof(dispSettings);

  if (0 != EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dispSettings)) {
    refreshRate = dispSettings.dmDisplayFrequency;
    if (refreshRate >= 100)  // we do not need 100fps - 50 is enough
      refreshRate /= 2;
  }
}

void Engine::onTargetChange(bool userInitiated) {
  displayPos.onTargetChange();
  texCache.onTargetChange();
  if (userInitiated)
    playbackTracer.delay(1);

  thread.invalidateWindow();
}
