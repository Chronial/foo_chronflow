#pragma once
#include "BlockingQueue.h"
#include "DbAlbumCollection.h"
#include "DbReloadWorker.h"
#include "DisplayPosition.h"
#include "FindAsYouType.h"
#include "PlaybackTracer.h"
#include "Renderer.h"
#include "TextureCache.h"
#include "cover_positions.h"
#include "utils.h"

class GLContext {
 public:
  explicit GLContext(class EngineWindow&);
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

  GLContext glContext;
  DbAlbumCollection db;
  FindAsYouType findAsYouType;
  ScriptedCoverPositions coverPos;
  DisplayPosition displayPos;
  TextureCache texCache;
  FpsCounter fpsCounter;
  Renderer renderer;
  PlaybackTracer playbackTracer;
  unique_ptr<DbReloadWorker> reloadWorker;

  Engine(EngineThread&, EngineWindow&);
  void mainLoop();
  void updateRefreshRate();
  void onTargetChange(bool userInitiated);

 private:
  void render();
  bool windowDirty = false;
  int refreshRate = 60;
  bool shouldStop = false;

 public:
  struct Messages;
};

using EM = Engine::Messages;

#include "engine_messages.h"
