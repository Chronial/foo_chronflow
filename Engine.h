#pragma once
#include "BlockingQueue.h"
#include "DbAlbumCollection.h"
#include "DbReloadWorker.h"
#include "DisplayPosition.h"
#include "FindAsYouType.h"
#include "PlaybackTracer.h"
#include "Renderer.h"
#include "TextureCache.h"

class GLContext {
 public:
  GLContext(class EngineWindow&);
};

class Engine {
 public:
  EngineWindow& window;
  class EngineThread& thread;

  GLContext glContext;
  DbAlbumCollection db;
  FindAsYouType findAsYouType;
  DisplayPosition displayPos;
  TextureCache texCache;
  Renderer renderer;
  PlaybackTracer playbackTracer;
  unique_ptr<DbReloadWorker> reloadWorker;

  Engine(EngineThread&, EngineWindow&);
  void mainLoop();
  void onPaint();
  void updateRefreshRate();
  void onTargetChange(bool userInitiated);

 private:
  bool doPaint = false;
  int timerResolution = 10;
  bool timerInPeriod = false;
  int refreshRate = 60;
  double afterLastSwap = 0;

 public:
  struct Messages;
};

using EM = Engine::Messages;

#include "engine_messages.h"
