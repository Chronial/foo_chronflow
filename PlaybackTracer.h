#pragma once
// clang-format off
#include "EngineThread.fwd.h"
#include "utils.h"
// clang-format on

namespace engine {

class PlaybackTracer {
  EngineThread& thread;

 public:
  explicit PlaybackTracer(EngineThread& thread) : thread(thread){};
  void delay(double extra_time);
  void moveToNowPlaying();
  void onPlaybackNewTrack(metadb_handle_ptr p_track);

 private:
  std::optional<Timer> delayTimer;
};
} // namespace
