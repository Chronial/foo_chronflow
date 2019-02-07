#pragma once
#include "Helpers.h"

class PlaybackTracer {
  class EngineThread& thread;

 public:
  PlaybackTracer(EngineThread&);
  void delay(double extra_time);
  void moveToNowPlaying();
  void onPlaybackNewTrack(metadb_handle_ptr p_track);

 private:
  std::optional<Timer> delayTimer;
};
