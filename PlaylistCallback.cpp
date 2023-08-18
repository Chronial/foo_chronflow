#include "stdafx.h"

// clang-format off
#include "PlaylistCallback.h"
//#include "EngineThread.fwd.h"
#include "ConfigData.h"
#include "Engine.h"
#include "EngineWindow.h"
// clang-format on

namespace engine {
using coverflow::configData;
using EM = engine::Engine::Messages;

void PlaylistCallback::on_item_focus_change(t_size p_playlist, t_size p_from,
                                            t_size p_to) {

  EngineThread* et = static_cast<EngineThread*>(this);
  bool bIsSrcOn = et->IsSourcePlaylistOn(p_playlist, PlSrcFilter::ANY_PLAYLIST);

  if (!bIsSrcOn) {
    return;
  }

  if (!et->GetCoverDispFlagU(DispFlags::FOLLOW_PL_SEL) && p_from != pfc_infinite) {
    return;
  }

  if (p_to != pfc_infinite) {
    src_state srcstate;
    et->GetState(srcstate);
    et->GetState(srcstate, false);
    srcstate.track_second.first = p_to;
    et->send<EM::SourceChangeMessage>(srcstate, (LPARAM)et->GetEngineWindowWnd());
    et->send<EM::ReloadCollection>((LPARAM)et->GetEngineWindowWnd());
  }
}

void PlaylistCallback::on_items_selection_change(t_size p_playlist,
                                                 const bit_array& p_affected,
                                                 const bit_array& p_state) {

  EngineThread* et = static_cast<EngineThread*>(this);

  bool bIsSrcOn = et->IsSourcePlaylistOn(p_playlist, PlSrcFilter::ANY_PLAYLIST);
  bool bIsWholeLib = et->IsWholeLibrary();
  bool bPlaylistModeGrouped = !bIsWholeLib && !et->GetCoverDispFlagU(DispFlags::SRC_PL_UNGROUPED);


  //if (!configData->IsSourcePlaylistOn(p_playlist, PlSrcFilter::ANY_PLAYLIST)) {
  if (!bIsSrcOn) {
    return;
  }

  if (!et->GetCoverDispFlagU(DispFlags::FOLLOW_PL_SEL)) {
    return;
  }

  metadb_handle_list p_all;
  metadb_handle_list p_selection;

  playlist_manager::get()->playlist_get_all_items(p_playlist, p_all);

  if (p_all.get_count() == 0)
    return; // pasting to an empty playlist?

  t_size ft_affected, ft_state, ft_myselection;

  ft_affected = p_affected.find_first(true, 0, p_all.get_count());
  ft_state = p_state.find_first(true, 0, p_all.get_count());

  if ((ft_state == pfc_infinite) || (ft_affected == pfc_infinite))
    return;  // undefined state

  if (ft_state >= p_all.get_count()) {
    // fp_state = size (first pass of a two-pass deselect/select)
    // or pasting into playlist?
    return;
  }

  if (ft_affected == ft_state) {
    // one-pass initial selection (stock playlis tbrowser)
    // or one-pass selection (first item on list)
    ft_myselection = ft_affected;
  } else
  if (ft_state ==  0) {
    // second pass of a two-pass-selection or one-pass selection
    // JSPlaylist does two-pass-selections, deselect/select
    ft_myselection = ft_affected;
  } else {
    //one-pass selection (stock playlistbrowser)
    ft_myselection = ft_state;
  }

  p_selection.add_item(p_all.get_item(ft_myselection));

  pfc::string8_fast_aggressive titleBuffer, keyBuffer, sortBuffer, db_keyBuffer;
  titleformat_object::ptr titleBuilder, keyBuilder, sortBuilder;
  pfc::stringcvt::string_wide_from_utf8_fast sortBufferWide;

  if (bPlaylistModeGrouped) {
    //grouped
    titleformat_compiler::get()->compile_safe_ex(keyBuilder, configData->Group);
    p_selection.get_item(0)->format_title(nullptr, keyBuffer, keyBuilder, nullptr);
    titleformat_compiler::get()->compile_safe_ex(sortBuilder, configData->Sort);
    p_selection.get_item(0)->format_title(nullptr, sortBuffer, sortBuilder, nullptr);
    sortBufferWide.convert(sortBuffer);
  } else {
    //ungrouped
    const DBUngroupedParams plparams;
    titleformat_compiler::get()->compile_safe_ex(titleBuilder, configData->SourcePlaylistNGTitle/*plparams.albumtitle*/);
    titleformat_compiler::get()->compile_safe_ex(keyBuilder, plparams.group);
    titleformat_compiler::get()->compile_safe_ex(sortBuilder, plparams.sort);
    p_selection.get_item(0)->format_title(nullptr, titleBuffer, titleBuilder, nullptr);
    p_selection.get_item(0)->format_title(nullptr, keyBuffer, keyBuilder, nullptr);
    p_selection.get_item(0)->format_title(nullptr, sortBuffer, sortBuilder, nullptr);
    sortBufferWide.convert(sortBuffer);

    char tmp_str[4];
    memset(&tmp_str, ' ', 3);
    tmp_str[3] = '\0';
    itoa(int(ft_myselection), tmp_str, 10);
    keyBuffer.add_string("|");
    keyBuffer.add_string(tmp_str);
  }

  auto target_albuminfo =
    static_cast<EngineThread*>(this)->sendSync<EM::GetTargetAlbum>().get();

  if (target_albuminfo && stricmp_utf8(target_albuminfo.value().pos.key.c_str(), keyBuffer) != 0) {
    DBPos sel_pos;
    sel_pos.key = keyBuffer;
    sel_pos.sortKey = sortBufferWide;
    AlbumInfo sel_albuminfo{titleBuffer.c_str(), sel_pos, p_selection};
    et->send<EM::MoveToAlbumMessage>(sel_albuminfo, false);
  }

};

void PlaylistCallback::on_items_added(t_size p_playlist, t_size p_start,
                                      const pfc::list_base_const_t<metadb_handle_ptr>& p_data,
                                      const bit_array& p_selection) {

  EngineThread* et = static_cast<EngineThread*>(this);

  bool bIsSrcOn = et->IsSourcePlaylistOn(p_playlist, PlSrcFilter::ANY_PLAYLIST);
  bool bIsWholeLib = et->IsWholeLibrary();
  bool bPlaylistFollowSelection = !bIsWholeLib && et->GetCoverDispFlagU(DispFlags::FOLLOW_PL_SEL);

  if (bIsSrcOn) {
    t_size plcount = playlist_manager::get()->playlist_get_item_count(p_playlist);

    if ((plcount == 1 && p_start == 0) || !bPlaylistFollowSelection) {
      et->send<EM::ReloadCollection>((LPARAM)et->GetEngineWindowWnd());
    } else {
    //todo: rev update on next on_focus_changed() ?
      et->send<EM::ReloadCollection>((LPARAM)et->GetEngineWindowWnd());
    }
  }
}

//called quite often if Library Viewer Selection is shown
void PlaylistCallback::on_items_reordered(t_size p_playlist,
                                          const t_size* p_order,
                                          t_size p_count) {

  EngineThread* et = static_cast<EngineThread*>(this);

  bool bIsSrcOn = et->IsSourcePlaylistOn(p_playlist, PlSrcFilter::ANY_PLAYLIST);

  if (bIsSrcOn) {
    et->send<EM::ReloadCollection>((LPARAM)et->GetEngineWindowWnd());
  }
}
void PlaylistCallback::on_items_removed(t_size p_playlist, const bit_array& p_mask,
                                        t_size p_old_count, t_size p_new_count) {

  EngineThread* et = static_cast<EngineThread*>(this);

  bool bIsSrcOn = et->IsSourcePlaylistOn(p_playlist, PlSrcFilter::ANY_PLAYLIST);
  bool bIsWholeLib = et->IsWholeLibrary();
  bool bPlaylistFollowSelection = !bIsWholeLib && et->GetCoverDispFlagU(DispFlags::FOLLOW_PL_SEL);

  if (bIsSrcOn)
    if (p_old_count != p_new_count) {
      if (p_new_count == 0 || !bPlaylistFollowSelection) {
        //not following selection or
        //empty playlist will not call on_focus_changed()
        et->send<EM::ReloadCollection>((LPARAM)et->GetEngineWindowWnd());
      } else {
        //update on next on_focus_changed()
      }
    }
}
// playlist activate and playlist removals
void PlaylistCallback::on_playlist_activate(t_size p_old, t_size p_new) {

  EngineThread* et = static_cast<EngineThread*>(this);

  bool bIsWholeLib = et->IsWholeLibrary();
  bool bIsPlaylistSource = et->GetCoverDispFlagU(DispFlags::SRC_PLAYLIST);
  bool bIsActivePlaylistSource = et->GetCoverDispFlagU(DispFlags::SRC_ACTPLAYLIST);
  bool bPlaylistFollowSelection = !bIsWholeLib && et->GetCoverDispFlagU(DispFlags::FOLLOW_PL_SEL);
  pfc::string8 coverSourcePlaylistName;
  et->GetPlaylistSource(coverSourcePlaylistName, false, false);
  pfc::string8 buffer;
  et->GetPlaylistSource(buffer, false, true);
  GUID guid_coverSourcePlaylist;
  guid_coverSourcePlaylist = pfc::GUID_from_text(buffer);
  et->GetPlaylistSource(buffer, true, true);
  GUID guid_coverSourceActivePlaylist;
  guid_coverSourceActivePlaylist = pfc::GUID_from_text(buffer);

  bool brefresh = false;
  src_state srcstate;
  // on playlist deletion callback is invoked twice...
  if (p_new == pfc_infinite) {
    // first callback after playlist removal (p_old is removed, p_new is undefined)
    // if p_old is zero we are deleting the last playlist and wont be a second
    // callback
    if (bIsPlaylistSource) {
      static_api_ptr_t<playlist_manager_v5> pm;
      pfc::string8 playlistname = coverSourcePlaylistName;
      t_size playlist = pm->find_playlist_by_guid(guid_coverSourcePlaylist);
      if (playlist == p_old) {
        GUID def_guid = pfc::guid_null;
        size_t def_ndx = pm->find_playlist(coverflow::default_SourcePlaylistName);
        bool bdefaultexists = def_ndx != pfc_infinite;
        if (bdefaultexists) {
          def_guid = pm->playlist_get_guid(def_ndx);
        }

        et->GetState(srcstate);
        et->SetPlaylistSource(coverflow::default_SourcePlaylistName, bIsActivePlaylistSource, false);
        et->SetPlaylistSource(pfc::print_guid(def_guid), bIsActivePlaylistSource, true);
        et->SetCoverDispFlagU(DispFlags::SRC_PLAYLIST, false);
        et->GetState(srcstate, false);
        brefresh = true;
      }
    }
  } else {
    // second callback after playlist removal, p_new is activated playlist, p_old is
    // undefined or normal playlist activation, p_new <> p_old, both p_new and p_old
    // defined

    if (bIsActivePlaylistSource) {

      static_api_ptr_t<playlist_manager_v5> pm;
      et->GetState(srcstate);
      pfc::string8 playlistname;
      pm->playlist_get_name(p_new, playlistname);
      GUID guid_playlist = pm->playlist_get_guid(p_new);
      et->SetPlaylistSource(playlistname, bIsActivePlaylistSource, false);
      et->SetPlaylistSource(pfc::print_guid(guid_playlist), bIsActivePlaylistSource, true);
      et->GetState(srcstate, false);

      brefresh = true;
    } else {
      if (bIsPlaylistSource) {
        bool bIsSrcOn = et->IsSourcePlaylistOn(p_new, PlSrcFilter::PLAYLIST);
        if (bIsSrcOn) {
          auto targetAlbum = et->sendSync<EM::GetTargetAlbum>().get();
          if (!targetAlbum) {
            brefresh = true;
          }
        }
      }
    }
  }
  if (brefresh) {
    if (bPlaylistFollowSelection) {
      et->send<EM::SourceChangeMessage>(srcstate, (LPARAM)et->GetEngineWindowWnd());
    }
    et->send<EM::ReloadCollection>((LPARAM)et->GetEngineWindowWnd());
  }
}
void PlaylistCallback::on_playlist_renamed(t_size p_index, const char* p_new_name,
                                           t_size p_new_name_len) {

  EngineThread* et = static_cast<EngineThread*>(this);

  bool bIsWholeLib = et->IsWholeLibrary();
  bool bIsActivePlaylistSource = et->GetCoverDispFlagU(DispFlags::SRC_ACTPLAYLIST);
  //todo: tidy up
  pfc::string8 coverSourcePlaylistName;
  et->GetPlaylistSource(coverSourcePlaylistName, false, false);
  pfc::string8 buffer;
  et->GetPlaylistSource(buffer, false, true);
  GUID guid_coverSourcePlaylist;
  guid_coverSourcePlaylist = pfc::GUID_from_text(buffer);
  et->GetPlaylistSource(buffer, true, true);
  GUID guid_coverSourceActivePlaylist;
  guid_coverSourceActivePlaylist = pfc::GUID_from_text(buffer);

  if (!bIsWholeLib) {
    static_api_ptr_t<playlist_manager_v5> pm;
    pfc::string8 srcPlaylistName;
    et->GetPlaylistSource(srcPlaylistName, bIsActivePlaylistSource, false);
    
    t_size playlist = pm->find_playlist(srcPlaylistName, srcPlaylistName.get_length());

    if (playlist == ~0) {
      GUID guid_playlist = pm->playlist_get_guid(p_index);
      et->SetPlaylistSource(p_new_name, bIsActivePlaylistSource, false);
      et->SetPlaylistSource(pfc::print_guid(guid_playlist), bIsActivePlaylistSource, true);
    }
  }
}
void PlaylistCallback::on_playlists_removed(const bit_array& p_mask, t_size p_old_count,
                                            t_size p_new_count) {

  EngineThread* et = static_cast<EngineThread*>(this);
  bool bIsWholeLib = et->IsWholeLibrary();

  if (!bIsWholeLib) {
    // manage state with no available playlist
    // other deletions are processed by on_playlist_activate(...)
    if (p_new_count == 0 /*|| bIsDeletingSrcOn*/) {
      et->send<EM::ReloadCollection>((LPARAM)et->GetEngineWindowWnd());
    }
  }
}

}  // namespace engine
