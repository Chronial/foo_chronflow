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

std::optional<AlbumInfo> EM::GetTrackAlbum::run(Engine& e, metadb_handle_ptr track) {
    // DBIter target;
    if (auto pos = e.db.getPosForTrack(track)) {
      auto iter = e.db.iterFromPos(pos.value());
      return e.db.getAlbumInfo(iter.value());
    }
    return std::nullopt;
}

void EM::ReloadCollection::run(Engine& e) {
  // This will abort any already running reload worker
//#ifdef DEBUG
  // PFC_ASSERT(e.thread.libraryVersion<2);
//#endif
  try {
    // stress crashed reloadWorker in playlist mode, need to release
    // test case: active playlist mode, fast mouse scroll through playlist tabs
    if (e.reloadWorker) {
      e.reloadWorker.release();
    }
  } catch (std::exception) {
  }

  e.reloadWorker = make_unique<DbReloadWorker>(e.thread);

  if (!e.reloadWorker) {
    PFC_ASSERT(true);
  }
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
// todo: items added and modified are turned off in source playlist mode
// otherwise items get added to selection
void EM::LibraryItemsAdded::run(Engine& e, metadb_handle_list tracks, t_uint64 version) {

  if (!configData->IsWholeLibrary() || configData->SourceLibrarySelectorLock == true)
    return;

  e.db.handleLibraryChange(version, DbAlbumCollection::items_added, std::move(tracks));
  e.cacheDirty = true;
  e.thread.invalidateWindow();
}
// todo: removing, should we check if source is playlist?
void EM::LibraryItemsRemoved::run(Engine& e, metadb_handle_list tracks,
                                  t_uint64 version) {
  e.db.handleLibraryChange(version, DbAlbumCollection::items_removed, std::move(tracks));
  e.cacheDirty = true;
  e.thread.invalidateWindow();
}
// todo: items added and modified are turned off in source playlist mode
// otherwise items get added to selection
void EM::LibraryItemsModified::run(Engine& e, metadb_handle_list tracks,
                                   t_uint64 version) {
  if (configData->SourcePlaylist || configData->SourceActivePlaylist)
    return;

  e.db.handleLibraryChange(version, DbAlbumCollection::items_modified, std::move(tracks));
  e.cacheDirty = true;
  e.thread.invalidateWindow();
}

void EM::SourceChangeMessage::run(Engine& e, bool waswholelib, bool wasgroup, bool nextwholelib, int track_pos) {
  // trying to fall back to the same album after next collection reload
  // the position is undetermined when entering ungrouped playlist mode
  // todo: should it be done in class DBWriter ?

  titleformat_object::ptr keyBuilder;
  titleformat_object::ptr sortBuilder;
  pfc::string8_fast_aggressive keyBuffer;
  pfc::string8_fast_aggressive sortBuffer;
  pfc::stringcvt::string_wide_from_utf8_fast sortBufferWide;

  DBPos pos = e.worldState.getTarget();
  std::optional dbiter = e.db.iterFromPos(pos);

  metadb_handle_list tracks;
  e.db.getTracks(dbiter.value(), tracks);

  if (waswholelib) {
    titleformat_compiler::get()->compile_safe_ex(keyBuilder, configData->Group);
    if (configData->SortGroup) {
      titleformat_compiler::get()->compile_safe_ex(sortBuilder, configData->Sort);
      tracks[0]->format_title(nullptr, sortBuffer, sortBuilder, nullptr);
      if (sortBuilder.is_valid())
          sortBufferWide.convert(sortBuffer);
      else
          sortBufferWide.convert(keyBuffer);
    }

    pos.key = keyBuffer.c_str();
    pos.sortKey = sortBufferWide.get_ptr();
  }
  else {
      if (wasgroup) {
          //todo: reposition moving to ungroup

      }
      else {
          //currently ungrouped, moving into grouped
          //works but not pretty

          titleformat_compiler::get()->compile_safe_ex(keyBuilder, configData->Group);
          tracks[0]->format_title(nullptr, keyBuffer, keyBuilder, nullptr);
          pos.key = keyBuffer.c_str();

          //sort
          DBPlaylistModeParams ps_params;
          titleformat_compiler::get()->compile_safe_ex(sortBuilder, ps_params.sort);
          tracks[0]->format_title(nullptr, sortBuffer, sortBuilder, nullptr);

          if (sortBuilder.is_valid()) {
              sortBufferWide.convert(sortBuffer);
              pos.sortKey = sortBufferWide.get_ptr();  //"La Bien Querida|2010|Recompensarte";
          }
          else {
              sortBufferWide.convert(keyBuffer);
              pos.sortKey = sortBufferWide.get_ptr();
          }
          pos.sortKey = sortBufferWide;
      }
  }
  e.worldState.setTarget(pos);
  e.worldState.hardSetCenteredPos(pos);
  e.worldState.update();
}
void EM::ReloadCollectionFromList::run(Engine& e, std::shared_ptr<metadb_handle_list> shared_selection) {
  // This will abort any already running reload worker
  try {
    //release or crash dbworker
    if (e.reloadWorker) {
      e.reloadWorker.release();
    }
  } catch (std::exception) {
  }
  t_size count = shared_selection->get_count();
  metadb_handle_list_ref p_selection = *shared_selection;
  //debug count = p_selection.get_count();

  e.reloadWorker = make_unique<DbReloadWorker>(e.thread, shared_selection);
  if (!e.reloadWorker) {
    PFC_ASSERT(true);
  }
  // Start spinner animation
  e.windowDirty = true;
}

}  // namespace engine
