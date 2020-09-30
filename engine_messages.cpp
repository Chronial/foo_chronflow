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

void EM::MoveToAlbumMessage::run(Engine& e, AlbumInfo album, bool userInitiated) {
  e.setTarget(album.pos, userInitiated);
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
  //todo: spinner animation & windowDirty causes flicker
  //when changing sources, removed by now
  //e.windowDirty = true;
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

void EM::ArtChangedMessage::run(Engine& e, bool prev, bool ctrl) {
  if (e.db.empty())
    return;
  e.findAsYouType.reset();

  e.cacheDirty = true;
  e.texCache.modifyCenterCache(prev, ctrl);
  //todo: this is a hack to invalidate texturecache
  //need to properly trigger GLUpload...
  //e.thread.invalidateWindow();
  e.delayTimer.emplace(0.3f, [&] {
    e.thread.invalidateWindow();
  });
}

//todo: these visual error messages are a hack, remove then
//as custom actions are now disabled in playlist mode
void EM::VisualErrorMessage::run(Engine& e) {
  static int oldHighlightWidth;
  if (oldHighlightWidth != 0) {
#ifdef DEBUG
    console::out() << "EM::VisualErrorMessage is busy";
#endif
    return;
  }
  oldHighlightWidth = configData->HighlightWidth;
  configData->HighlightWidth = configData->HighlightWidth + 2;
  e.thread.invalidateWindow();
  e.delayTimer.emplace(0.3f, [&] {
    configData->HighlightWidth = configData->HighlightWidth - 2;
    e.thread.invalidateWindow();
    oldHighlightWidth = 0;
  });
}

bool GetKeys(metadb_handle_ptr newtarget, pfc::string_base & keyBuffer,
  pfc::stringcvt::string_wide_from_utf8_fast & sortBufferWide,
  pfc::string8 group, pfc::string8 sort,
  bool sortgroup, t_size ngpos) {
  if (newtarget.is_empty())
    return false;
  try {
    titleformat_object::ptr keyBuilder;
    titleformat_object::ptr sortBuilder;
    pfc::string8_fast_aggressive sortBuffer;

    titleformat_compiler::get()->compile_safe_ex(keyBuilder, group /*configData->Group*/);
    if (/*configData->SortGroup*/sortgroup) {
      titleformat_compiler::get()->compile_safe_ex(sortBuilder, sort/*configData->Sort*/);
      newtarget->format_title(nullptr, keyBuffer, keyBuilder, nullptr);
      if (sort.equals("NONE")) {
        char tmp_str[4];
        memset(tmp_str, ' ', 3);
        tmp_str[3] = '\0';
        itoa(ngpos, tmp_str, 10);
        keyBuffer.replace_string("|[UNKNOWN FUNCTION]", "");
        keyBuffer = keyBuffer << "|" << tmp_str;
      }

      newtarget->format_title(nullptr, sortBuffer, sortBuilder, nullptr);
      if (sortBuilder.is_valid())
        sortBufferWide.convert(sortBuffer);
      else
        sortBufferWide.convert(keyBuffer);
    }
    return true;
  } catch (std::exception) {
    return false;
  }
}

void EM::SourceChangeMessage::run(Engine& e, src_state srcstate) {

  DBPos pos = e.worldState.getTarget();
  std::optional dbiter = e.db.iterFromPos(pos);

  if (dbiter.has_value()) {
    metadb_handle_list tracks;
    e.db.getTracks(dbiter.value(), tracks);
    if (tracks.get_count() == 0)
      return;
  } else return;

  bool bskip = false;
  bool bgetkeys = false;
  pfc::string8_fast_aggressive keyBuffer;
  pfc::stringcvt::string_wide_from_utf8_fast sortBufferWide;

  pfc::string8 group;
  pfc::string8 sort;
  bool sortgroup;
  metadb_handle_ptr newtarget;
  t_size secondpos;

  if (srcstate.wholelib.first) {
    // from library...

    if (srcstate.wholelib.second) {
      // library to library
      sortgroup = "";
      secondpos = 0;
      bskip = true;
    }
    else {
      // library to playlist...
      if (srcstate.grouped.second) {
        // library to playlist grouped
        group = configData->Group;
        sort = configData->Sort;
        sortgroup = configData->SortGroup;
        newtarget = srcstate.track_second.second;
        secondpos = srcstate.track_second.first;
      } else {
        // library to ungrouped playlist
        DBPlaylistModeParams plparams;
        group = plparams.group;
        group += "|$hi()";
        sort = plparams.sort;
        sortgroup = plparams.sortgroup;
        newtarget = srcstate.track_second.second;
        secondpos = srcstate.track_second.first;
      }
    }
  }
  else {
    //from playlist...
    if (srcstate.wholelib.second) {
      // playlist g/u to library
      group = configData->Group;
      sort = configData->Sort;
      sortgroup = configData->SortGroup;
      newtarget = srcstate.track_first.second;
      secondpos = srcstate.track_first.first;
    } else {
      // from playlist to playlist...
      if (srcstate.grouped.first) {
        // grouped playlist...
        if (srcstate.grouped.second) {
          // grouped playlist to grouped playlist
          group = configData->Group;
          sort = configData->Sort;
          sortgroup = configData->SortGroup;
          newtarget = srcstate.track_second.second;
          secondpos = srcstate.track_second.first;
        } else {
          // grouped to ungrouped playlist
          DBPlaylistModeParams plparams;
          group = plparams.group;
          group += "|$hi()";
          sort = plparams.sort;
          sortgroup = plparams.sortgroup;
          newtarget = srcstate.track_first.second;
          secondpos = srcstate.track_first.first;
        }
      }
      else {
        //from ungrouped playlist...
        if (srcstate.grouped.second) {
          // from ungrouped playlist to grouped playlist
          bskip = true;
          sortgroup = "";
          secondpos = 0;
          //group = configData->Group;
          //sort = configData->Sort;
          //sortgroup = configData->SortGroup;
          //newtarget = srcstate.track_first.second;
          //secondpos = srcstate.track_second.first;
        } else {
          // ungrouped playlist to ungrouped playlist
          DBPlaylistModeParams plparams;
          group = plparams.group;
          group += "|$hi()";
          sort = plparams.sort;
          sortgroup = plparams.sortgroup;
          newtarget = srcstate.track_second.second;
          secondpos = srcstate.track_second.first;
        }
      }
    }
  }

  if (!bskip) {
    bgetkeys = GetKeys(
        newtarget, keyBuffer, sortBufferWide, group, sort, sortgroup, secondpos);

    pos.key = keyBuffer.c_str();
    pos.sortKey = sortBufferWide.get_ptr();
  }

  if (bgetkeys) {

    //e.worldState.hardSetCenteredPos(pos);
    e.worldState.setTarget(pos);

    //configData->sessionSelectedCover.set_string(pos.key.c_str());

    //ori
    //e.worldState.setTarget(pos);
    //e.worldState.hardSetCenteredPos(pos);
    //e.worldState.update();
  }
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
  e.reloadWorker = make_unique<DbReloadWorker>(e.thread, shared_selection);
  if (!e.reloadWorker) {
    PFC_ASSERT(true);
  }
  // Start spinner animation
  // e.windowDirty = true;
}

}  // namespace engine
