#include "Engine.h"

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
  static const bool glad =
      (0 != gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)));
  if (!(glad && GLAD_GL_VERSION_2_1 && GLAD_GL_EXT_texture_filter_anisotropic &&
        GLAD_GL_EXT_bgra)) {
    throw std::exception("Glad failed to initialize OpenGl");
  }

  glShadeModel(GL_SMOOTH);
  glClearDepth(1.0f);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glHint(GL_TEXTURE_COMPRESSION_HINT, GL_FASTEST);
  glEnable(GL_TEXTURE_2D);

  glFogi(GL_FOG_MODE, GL_EXP);
  glFogf(GL_FOG_DENSITY, 5);
  glHint(GL_FOG_HINT, GL_NICEST);  // Per-Pixel Fog Calculation
  glFogi(GL_FOG_COORD_SRC, GL_FOG_COORD);  // Set Fog Based On Vertice Coordinates
}

HighTimerResolution::HighTimerResolution() {
  timeBeginPeriod(get());
}

HighTimerResolution::~HighTimerResolution() {
  timeEndPeriod(get());
}

int HighTimerResolution::get() {
  static int resolution = [] {
    TIMECAPS tc;
    if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != MMSYSERR_NOERROR)
      throw(std::runtime_error("timeGetDevCaps failed"));
    return std::min(std::max(tc.wPeriodMin, 1u), tc.wPeriodMax);
  }();
  return resolution;
}

Engine::Engine(EngineThread& thread, EngineWindow& window)
    : window(window), thread(thread), glContext(window), findAsYouType(*this),
      displayPos(db), texCache(thread, db, coverPos), renderer(*this),
      playbackTracer(thread) {}

void Engine::mainLoop() {
  updateRefreshRate();
  EM::ReloadCollection().execute(*this);

  double lastSwapTime = 0;
  std::optional<HighTimerResolution> timerResolution;
  while (!shouldStop) {
    while (!shouldStop) {
      std::unique_ptr<engine_messages::Message> msg = nullptr;
      if (windowDirty) {
        auto maybeMsg = thread.messageQueue.popMaybe();
        if (!maybeMsg)
          break;
        msg = std::move(maybeMsg.value());
      } else {
        msg = thread.messageQueue.pop();
      }
      msg->execute(*this);
    }

    if (!timerResolution)
      timerResolution.emplace();

    // Housekeeping
    texCache.uploadTextures();
    texCache.trimCache();

    // Update state
    displayPos.update();

    // Render
    render();
    windowDirty = displayPos.isMoving() || renderer.wasMissingTextures;

    // Handle V-Sync
    renderer.ensureVSync(cfgVSyncMode != VSYNC_SLEEP_ONLY);
    if (cfgVSyncMode == VSYNC_AND_SLEEP || cfgVSyncMode == VSYNC_SLEEP_ONLY) {
      double currentTime = Helpers::getHighresTimer();
      // time we have - time we already have spend
      int sleepTime = int((1000.0 / refreshRate) - 1000 * (currentTime - lastSwapTime));
      if (cfgVSyncMode == VSYNC_AND_SLEEP) {
        sleepTime -= 2 * timerResolution->get();
      } else {
        sleepTime -= timerResolution->get();
      }
      if (sleepTime >= 0) {
        Sleep(sleepTime);
      }
    }
    window.swapBuffers();
    glFinish();  // Wait for GPU
    lastSwapTime = Helpers::getHighresTimer();

    if (!windowDirty)
      timerResolution.reset();
  }
  // Synchronize with GPU before shutdown to avoid crashes
  glFinish();
}

void Engine::render() {
  TRACK_CALL_TEXT("EngineThread::render");
  double frameStart = Helpers::getHighresTimer();
  renderer.drawFrame();
  glFinish();
  double frameEnd = Helpers::getHighresTimer();
  fpsCounter.recordFrame(frameStart, frameEnd);
}

void Engine::updateRefreshRate() {
  DEVMODE dispSettings;
  ZeroMemory(&dispSettings, sizeof(dispSettings));
  dispSettings.dmSize = sizeof(dispSettings);

  if (0 != EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &dispSettings)) {
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
