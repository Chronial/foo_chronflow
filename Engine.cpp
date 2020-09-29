// clang-format off
#include "Engine.h"

#include "EngineThread.h" //(1)
#include "EngineWindow.h" //(2)

#include "renderer.h"
// clang-format on

#include "MyActions.h"
#include "ConfigData.h"

namespace engine {

using coverflow::configData;
using namespace render;
using namespace ::engine_messages;

using EM = engine::Engine::Messages;

#ifdef _DEBUG
void GLAPIENTRY glMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                  GLsizei length, const GLchar* message,
                                  const void* userParam) {
  console::out() << "GL: " << message;
  if (type == GL_DEBUG_TYPE_ERROR || severity == GL_DEBUG_SEVERITY_HIGH)
    __debugbreak();
}
#endif

GLContext::GLContext(EngineWindow& window) {
  window.makeContextCurrent();
  // Static, since we only want to initialize the extensions once.
  // This also takes care of multithread synchronization.
  static const bool glad =
      (0 != gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)));
  if (!glad)
    throw std::exception("Glad could not initialize OpenGl");
  if (!GLAD_GL_VERSION_2_1)
    throw std::exception("OpenGL 2.1 is not supported");
  if (!GLAD_GL_EXT_texture_filter_anisotropic)
    throw std::exception("Missing support for anisotropic textures");

  IF_DEBUG(glEnable(GL_DEBUG_OUTPUT));
  IF_DEBUG(glDebugMessageCallback(glMessageCallback, 0));

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

GLContext::~GLContext() {
  glFinish();
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

Engine::Engine(EngineThread& thread, EngineWindow& window, StyleManager& styleManager)
    : window(window), thread(thread), styleManager(styleManager), glContext(window),
      findAsYouType(*this), coverPos(std::move(configData->sessionCompiledCPInfo.get().second)), worldState(db),
      texCache(thread, db, coverPos), playbackTracer(thread) {
        renderer = make_unique<Renderer>(*this);
}

void Engine::mainLoop() {
  updateRefreshRate();
  EM::ReloadCollection().execute(*this);

  double lastSwapEnd = 0;
  double swapEstimate = 1;
  std::optional<HighTimerResolution> timerResolution;
  while (!shouldStop) {
    if (!windowDirty && !cacheDirty)
      thread.messageQueue.wait();
    if (auto msg = thread.messageQueue.popMaybe()) {
      msg.value()->execute(*this);
    } else if (cacheDirty) {
      texCache.startLoading(worldState.getTarget());
      cacheDirty = false;
    } else if (windowDirty) {
      fpsCounter.startFrame();
      if (!timerResolution)
        timerResolution.emplace();
      texCache.setPriority(true);

      // Housekeeping
      texCache.uploadTextures();
      texCache.trimCache();

      // Update state
      worldState.update();

      // Render
      renderer->drawFrame();
      glFinish();
      fpsCounter.endFrame();

      windowDirty = worldState.isMoving() || renderer->wasMissingTextures || reloadWorker;

      // Handle V-Sync
      renderer->ensureVSync(configData->VSyncMode != VSYNC_SLEEP_ONLY);
      if (configData->VSyncMode == VSYNC_AND_SLEEP || configData->VSyncMode == VSYNC_SLEEP_ONLY) {
        double currentTime = time();
        double sleepTime =
            (1.0 / refreshRate) - (currentTime - lastSwapEnd) - swapEstimate;
        sleepTime -= 0.002 * timerResolution->get();
        if (sleepTime >= 0.001 * timerResolution->get()) {
          SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
          Sleep(int(1000 * std::min(sleepTime, 1.0)));
          SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
        }
      }
      double swapStart = time();
      window.swapBuffers();
      glFinish();  // Wait for GPU
      lastSwapEnd = time();
      swapEstimate = 0.2 * (lastSwapEnd - swapStart) + 0.8 * swapEstimate;

      texCache.setPriority(windowDirty);
      if (!windowDirty) {
        timerResolution.reset();
        fpsCounter.flush();
      }
    }
  }
  // Synchronize with GPU before shutdown to avoid crashes
  glFinish();
}

void Engine::updateRefreshRate() {
  DEVMODE dispSettings;
  ZeroMemory(&dispSettings, sizeof(dispSettings));
  dispSettings.dmSize = sizeof(dispSettings);

  if (0 != EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &dispSettings)) {
    refreshRate = dispSettings.dmDisplayFrequency;
  }
}

void Engine::setTarget(DBPos target, bool userInitiated) {
  if (auto dbIter = db.iterFromPos(target)) {
    metadb_handle_list tracks;
    db.getTracks(dbIter.value(), tracks);
    thread.runInMainThread([target, tracks = std::move(tracks), userInitiated, &engineWindow = window]{
      //console::out() << "Engine::setTarget key:" << target.key << " sk:" << target.sortKey.c_str() <<", userInitiated: " << userInitiated ? "true" : "false";
      engineWindow.setSelection(tracks, userInitiated);
    });
  }

  //todo: revise custom art refresh
  if (configData->CenterArt != configData->CustomCoverFrontArt) {
    configData->CenterArt = configData->CustomCoverFrontArt;
  }
  worldState.setTarget(target);
  cacheDirty = true;
  if (userInitiated)
    playbackTracer.delay(1);
  thread.invalidateWindow();

}
} // namespace engine
