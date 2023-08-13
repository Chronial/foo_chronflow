// clang-format off
#include "EngineThread.h"
#include "Engine.h"
#include "PlaybackTracer.h"
#include "ConfigData.h"
// clang-format on
namespace engine {
using coverflow::configData;
using EM = engine::Engine::Messages;

void PlaybackTracer::delay(double extra_time) {
  delayTimer.emplace(configData->CoverFollowDelay + extra_time, [&] {
    this->thread.send<EM::Run>([&] {
      if (configData->CoverFollowsPlayback)
        moveToNowPlaying(NULL);
      delayTimer.reset();
    });
  });
}

void PlaybackTracer::moveToNowPlaying(HWND wnd) {
  this->thread.runInMainThread([&, wnd] {
    metadb_handle_ptr nowPlaying;
    bool bnowplaying = playback_control_v2::get()->get_now_playing(nowPlaying);
    if (!bnowplaying)
      return;

    this->thread.send<EM::MoveToCurrentTrack>(nowPlaying, false, (LPARAM)wnd);

  });
}

void PlaybackTracer::onPlaybackNewTrack(metadb_handle_ptr /*p_track*/) {
  if (!configData->CoverFollowsPlayback || delayTimer.has_value())
    return;
  moveToNowPlaying();
}
} // namespace
