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
        moveToNowPlaying();
      delayTimer.reset();
    });
  });
}

void PlaybackTracer::moveToNowPlaying() {
  this->thread.runInMainThread([&] {
    bool bmove = false;
    metadb_handle_ptr nowPlaying;
    bool bnowplaying = playback_control_v2::get()->get_now_playing(nowPlaying);
    if (!bnowplaying)
      return;

    t_size tsNowPos;
    if (!configData->IsWholeLibrary()) {
      metadb_handle_list nowSelected;
      pfc::string8 strSourceList = configData->InSourePlaylistGetName();
      t_size tsSourceList = playlist_manager::get()->find_playlist(strSourceList);
      if (tsSourceList == pfc_infinite) {
        bmove = true;
      } else {
        bmove = playlist_manager::get()->playlist_find_item(tsSourceList, nowPlaying, tsNowPos);
      }
    } else if (configData->SourceLibrarySelectorLock) {
      //todo: move to now playing in selection lock mode
      bmove = false;
    } else {
      bmove = true;
    }
    if (bmove)
      this->thread.send<EM::MoveToCurrentTrack>(nowPlaying);

  });
}

void PlaybackTracer::onPlaybackNewTrack(metadb_handle_ptr /*p_track*/) {
  if (!configData->CoverFollowsPlayback || delayTimer.has_value())
    return;
  moveToNowPlaying();
}
} // namespace
