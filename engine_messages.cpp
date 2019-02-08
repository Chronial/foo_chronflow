#include "Engine.h"

#include "EngineThread.h"
#include "config.h"

void EM::RedrawMessage::run(Engine& e) {
  e.doPaint = true;
}

void EM::DeviceModeMessage::run(Engine& e) {
  e.updateRefreshRate();
}

void EM::WindowResizeMessage::run(Engine& e, int width, int height) {
  e.renderer.resizeGlScene(width, height);
}

void EM::ChangeCPScriptMessage::run(Engine& e, pfc::string8 script) {
  pfc::string8 tmp;
  e.renderer.coverPos.setScript(script, tmp);
  e.renderer.setProjectionMatrix();
  e.doPaint = true;
}

void EM::WindowHideMessage::run(Engine& e) {
  // texCache.isPaused = true;
  if (cfgEmptyCacheOnMinimize) {
    e.texCache.clearCache();
  }
}

void EM::WindowShowMessage::run(Engine& e) {
  // texCache.isPaused = false;
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
  if (!e.db.getCount())
    return;

  if (!moveToEnd) {
    e.db.moveTargetBy(moveBy);
    e.onTargetChange(true);
  } else {
    CollectionPos newTarget;
    if (moveBy > 0) {
      newTarget = --e.db.end();
    } else {
      newTarget = e.db.begin();
    }
    e.db.setTargetPos(newTarget);
    e.onTargetChange(true);
  }
}

void EM::MoveToCurrentTrack::run(Engine& e, metadb_handle_ptr track) {
  CollectionPos target;
  if (e.db.getAlbumForTrack(track, target)) {
    e.db.setTargetPos(target);
    e.onTargetChange(false);
  }
}

void EM::MoveToAlbumMessage::run(Engine& e, std::string groupString) {
  if (!e.db.getCount())
    return;

  e.db.setTargetByName(groupString);
  e.onTargetChange(true);
}

std::optional<AlbumInfo> EM::GetAlbumAtCoords::run(Engine& e, int x, int y) {
  int offset;
  if (!e.renderer.offsetOnPoint(x, y, offset)) {
    return std::nullopt;
  }
  CollectionPos pos = e.displayPos.getOffsetPos(offset);
  return e.db.getAlbumInfo(pos);
}

std::optional<AlbumInfo> EM::GetTargetAlbum::run(Engine& e) {
  if (!e.db.getCount())
    return std::nullopt;
  auto pos = e.db.getTargetPos();
  return e.db.getAlbumInfo(pos);
}

void EM::ReloadCollection::run(Engine& e) {
  if (e.reloadWorker)
    return;  // already running
  e.reloadWorker = make_unique<DbReloadWorker>(e.thread);
}

void EM::CollectionReloadedMessage::run(Engine& e) {
  e.db.onCollectionReload(std::move(*e.reloadWorker));
  e.reloadWorker.reset();
  CollectionPos newTargetPos = e.db.getTargetPos();
  e.displayPos.hardSetCenteredPos(newTargetPos);
  e.texCache.onCollectionReload();
  e.thread.invalidateWindow();
}

void EM::PlaybackNewTrack::run(Engine& e, metadb_handle_ptr track) {
  e.playbackTracer.onPlaybackNewTrack(track);
}
