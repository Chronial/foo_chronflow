#include "Engine.h"

#include "EngineThread.h"
#include "config.h"

void EM::StopThread::run(Engine& e) {
  e.shouldStop = true;
}

void EM::RedrawMessage::run(Engine& e) {
  e.windowDirty = true;
}

void EM::DeviceModeMessage::run(Engine& e) {
  e.updateRefreshRate();
}

void EM::WindowResizeMessage::run(Engine& e, int width, int height) {
  e.renderer.resizeGlScene(width, height);
}

void EM::ChangeCPScriptMessage::run(Engine& e, pfc::string8 script) {
  pfc::string8 tmp;
  e.coverPos.setScript(script, tmp);
  e.renderer.setProjectionMatrix();
  e.texCache.startLoading(e.worldState.getTarget());
  e.windowDirty = true;
}

void EM::WindowHideMessage::run(Engine& e) {
  e.texCache.pauseLoading();
  if (cfgEmptyCacheOnMinimize) {
    e.texCache.clearCache();
  }
}

void EM::WindowShowMessage::run(Engine& e) {
  if (cfgEmptyCacheOnMinimize) {
    e.texCache.startLoading(e.worldState.getTarget());
  }
  e.texCache.resumeLoading();
}

void EM::TextFormatChangedMessage::run(Engine& e) {
  e.renderer.textDisplay.clearCache();
}

void EM::CharEntered::run(Engine& e, WPARAM wParam) {
  e.findAsYouType.onChar(wParam);
}

void EM::Run::run(Engine& /*e*/, std::function<void()> f) {
  f();
}

void EM::MoveToNowPlayingMessage::run(Engine& e) {
  e.playbackTracer.moveToNowPlaying();
}

void EM::MoveTargetMessage::run(Engine& e, int moveBy, bool moveToEnd) {
  if (e.db.empty())
    return;
  if (!moveToEnd) {
    auto target = e.worldState.getTarget();
    e.setTarget(e.db.movePosBy(target, moveBy), true);
  } else {
    DBIter newTarget;
    if (moveBy > 0) {
      newTarget = --e.db.end();
    } else {
      newTarget = e.db.begin();
    }
    e.setTarget(e.db.posFromIter(newTarget), true);
  }
}

void EM::MoveToCurrentTrack::run(Engine& e, metadb_handle_ptr track) {
  DBIter target;
  if (auto pos = e.db.getPosForTrack(track)) {
    e.setTarget(pos.value(), false);
  }
}

void EM::MoveToAlbumMessage::run(Engine& e, AlbumInfo album) {
  e.setTarget(album.pos, true);
}

std::optional<AlbumInfo> EM::GetAlbumAtCoords::run(Engine& e, int x, int y) {
  return e.renderer.albumAtPoint(x, y);
}

std::optional<AlbumInfo> EM::GetTargetAlbum::run(Engine& e) {
  auto iter = e.db.iterFromPos(e.worldState.getTarget());
  if (!iter)
    return std::nullopt;
  return e.db.getAlbumInfo(iter.value());
}

void EM::ReloadCollection::run(Engine& e) {
  if (e.reloadWorker)
    return;  // already running
  e.reloadWorker = make_unique<DbReloadWorker>(e.thread);
}

void EM::CollectionReloadedMessage::run(Engine& e) {
  e.db.onCollectionReload(std::move(*e.reloadWorker));
  e.reloadWorker.reset();
  e.texCache.onCollectionReload();
  e.texCache.startLoading(e.worldState.getTarget());
  e.thread.invalidateWindow();
}

void EM::PlaybackNewTrack::run(Engine& e, metadb_handle_ptr track) {
  e.playbackTracer.onPlaybackNewTrack(track);
}

void EM::LibraryItemsAdded::run(Engine& e, metadb_handle_list tracks, t_uint64 version) {
  e.db.handleLibraryChange(version, DbAlbumCollection::items_added, std::move(tracks));
  e.texCache.startLoading(e.worldState.getTarget());
  e.thread.invalidateWindow();
}
void EM::LibraryItemsRemoved::run(Engine& e, metadb_handle_list tracks,
                                  t_uint64 version) {
  e.db.handleLibraryChange(version, DbAlbumCollection::items_removed, std::move(tracks));
  e.texCache.startLoading(e.worldState.getTarget());
  e.thread.invalidateWindow();
}
void EM::LibraryItemsModified::run(Engine& e, metadb_handle_list tracks,
                                   t_uint64 version) {
  e.db.handleLibraryChange(version, DbAlbumCollection::items_modified, std::move(tracks));
  e.texCache.startLoading(e.worldState.getTarget());
  e.thread.invalidateWindow();
}
