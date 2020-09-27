// clang-format off
#include "ConfigData.h"
#include "Engine.h"
#include "Renderer.h"
// clang-format on

namespace engine {

using coverflow::configData;
using db::db_structure::DBIter;

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
  e.renderer->resizeGlScene(width, height);
}

void EM::ChangeCoverPositionsMessage::run(Engine& e,
                std::shared_ptr<CompiledCPInfo> cInfo) {
  e.coverPos = ScriptedCoverPositions(cInfo);
  e.renderer->setProjectionMatrix();
  e.cacheDirty = true;
  e.thread.invalidateWindow();
}

void EM::WindowHideMessage::run(Engine& e) {
  e.texCache.pauseLoading();
  if (configData->EmptyCacheOnMinimize) {
    e.texCache.clearCache();
  }
}

void EM::WindowShowMessage::run(Engine& e) {
  if (configData->EmptyCacheOnMinimize) {
    e.cacheDirty = true;
  }
  e.texCache.resumeLoading();
}

void EM::TextFormatChangedMessage::run(Engine& e) {
  e.renderer->textDisplay.clearCache();
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
  e.findAsYouType.reset();
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
  // DBIter target;
  if (auto pos = e.db.getPosForTrack(track)) {
    e.setTarget(pos.value(), false);
  }
}

void EM::MoveToAlbumMessage::run(Engine& e, AlbumInfo album) {
  e.setTarget(album.pos, true);
}

std::optional<AlbumInfo> EM::GetAlbumAtCoords::run(Engine& e, int x, int y) {
  return e.renderer->albumAtPoint(x, y);
}

std::optional<AlbumInfo> EM::GetTargetAlbum::run(Engine& e) {
  auto iter = e.db.iterFromPos(e.worldState.getTarget());
  if (!iter)
    return std::nullopt;
  return e.db.getAlbumInfo(iter.value());
}

void EM::ReloadCollection::run(Engine& e) {
  // This will abort any already running reload worker
  e.reloadWorker = make_unique<DbReloadWorker>(e.thread);
  // Start spinner animation
  e.windowDirty = true;
}

void EM::CollectionReloadedMessage::run(Engine& e) {
  if (!e.reloadWorker || !e.reloadWorker->completed)
    return;
  e.db.onCollectionReload(std::move(e.reloadWorker->db));
  e.reloadWorker.reset();
  e.texCache.onCollectionReload();
  e.cacheDirty = true;
  e.thread.invalidateWindow();
}

void EM::PlaybackNewTrack::run(Engine& e, metadb_handle_ptr track) {
  e.playbackTracer.onPlaybackNewTrack(track);
}

void EM::LibraryItemsAdded::run(Engine& e, metadb_handle_list tracks, t_uint64 version) {
  e.db.handleLibraryChange(version, DbAlbumCollection::items_added, std::move(tracks));
  e.cacheDirty = true;
  e.thread.invalidateWindow();
}
void EM::LibraryItemsRemoved::run(Engine& e, metadb_handle_list tracks,
                                  t_uint64 version) {
  e.db.handleLibraryChange(version, DbAlbumCollection::items_removed, std::move(tracks));
  e.cacheDirty = true;
  e.thread.invalidateWindow();
}
void EM::LibraryItemsModified::run(Engine& e, metadb_handle_list tracks,
                                   t_uint64 version) {
  e.db.handleLibraryChange(version, DbAlbumCollection::items_modified, std::move(tracks));
  e.cacheDirty = true;
  e.thread.invalidateWindow();
}
