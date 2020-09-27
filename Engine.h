#pragma once
// clang-format off
#include "EngineThread.fwd.h"
#include "Renderer.fwd.h"
#include "DbAlbumCollection.h"
#include "DbReloadWorker.h"

#include "TextureCache.h"
#include "EngineWindow.fwd.h" // (optional fwd)
// clang-format on
#include "FindAsYouType.h"
#include "cover_positions.h"
#include "world_state.h"
#include "style_manager.h"
#include "PlaybackTracer.h"

namespace engine {

using namespace worldstate;

using db::DbReloadWorker;
using render::Renderer;
using render::TextureCache;
using render::StyleManager;

class GLContext {
 public:
  explicit GLContext(class EngineWindow&);
  NO_MOVE_NO_COPY(GLContext);
  ~GLContext();
};

class HighTimerResolution {
 public:
  HighTimerResolution();
  NO_MOVE_NO_COPY(HighTimerResolution);
  ~HighTimerResolution();
  int get();
};

class Engine {
 public:
  EngineWindow& window;
  class EngineThread& thread;
  class StyleManager& styleManager;

  GLContext glContext;
  DbAlbumCollection db;
  FindAsYouType findAsYouType;
  ScriptedCoverPositions coverPos;
  WorldState worldState;
  TextureCache texCache;
  FpsCounter fpsCounter;
  Renderer renderer;
  PlaybackTracer playbackTracer;
  unique_ptr<DbReloadWorker> reloadWorker;

  Engine(EngineThread&, EngineWindow&, StyleManager& styleManager);
  void mainLoop();
  void updateRefreshRate();
  void setTarget(DBPos target, bool userInitiated);

 private:
  bool windowDirty = false;
  bool cacheDirty = true;
  int refreshRate = 60;
  std::atomic<bool> shouldStop = false;

 public:
  struct Messages;
};
} // namespace

using EM = engine::Engine::Messages;
#include "engine_messages.h"
