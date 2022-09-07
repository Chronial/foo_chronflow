#include "MyActions.h"

#include "ConfigData.h"

using coverflow::configData;
using coverflow::CoverConfigMap;

// link reqs.: version.lib
bool isInactivePlaylistPlayFixed(int amajor, int aminor, int arevision /*, int abuild*/) {
#ifdef _WIN64
  return true;
#else
  TCHAR szVersionFile[MAX_PATH];
  GetModuleFileName(NULL, szVersionFile, MAX_PATH);
  return isCheckBaseVersion(szVersionFile, amajor, aminor, arevision);
#endif
}

void CustomAction::NotifyRejected(HWND hwnd) {
  // MessageBeep(0xFFFFFFFF);
  // SendMessage(hwnd, WM_MYACTIONS_CANCELED, 0, 0);
}

bool CustomAction::availablePlaylist(t_size playlist) {
  bool bres = true;
  if (playlist == ~0)
    bres = false;
  else if (playlist_manager::get()->playlist_lock_is_present(playlist))
    bres = false;
  return bres;
}

void CustomAction::DoPlayItem(t_size p_item, t_size playlist,
                              const pfc::bit_array_bittable& p_mask, int flag) {

  t_size p_inspos = p_mask.find_first(true, 0, p_mask.size());

  if (p_inspos == pfc_infinite) return; //nothing to do

  static_api_ptr_t<playlist_manager> pm;
  t_size uiplaylist = ~0;
  bool b_need_queue = false;
  bool b_target_not_active = false;

  uiplaylist = pm->get_active_playlist();
  b_target_not_active = (uiplaylist != playlist);

  if (!BlockFlagEnabled(flag, ACT_ACTIVATE))
    b_need_queue = b_target_not_active && !isInactivePlaylistPlayFixed(1, 6, 4);

  metadb_handle_list tracks;

  try {
      pm->playlist_get_items(playlist, tracks, p_mask);
      if (tracks.get_count() > 0) //todo: test exceptions for next line...
          pm->playlist_set_focus_by_handle(playlist, tracks[0]);
      else
          return;
  }
  catch (std::exception e) {
  }

  if (!b_need_queue) {
    pm->playlist_execute_default_action(playlist, p_inspos);
  } else {
    // We should queue (activate playlist option is off and fb2k < v1.6.4
  }
}

// returns start position for new items, -1 for no changes
t_size CustomAction::DoAddReplace(const pfc::list_base_const_t<metadb_handle_ptr>& tracks,
                                  t_size playlist, int flag, t_size lastselected,
                                  pfc::list_t<metadb_handle_ptr>& updateTracks,
                                  bit_array_bittable& updateMask) {

  bool bres = false;
  updateTracks = tracks;

  static_api_ptr_t<playlist_manager> pm;

  // previous item position
  int position;
  t_size prevLength = pm->playlist_get_item_count(playlist);

  t_size playlistPlaying;
  t_size itemPlaying;
  bool wasPlaying;
  bool wasPlayingValid = pm->get_playing_item_location(&playlistPlaying, &itemPlaying);

  if (wasPlayingValid) {
    wasPlaying = (playlistPlaying == playlist);
    itemPlaying = wasPlaying ? itemPlaying : ~0;
  } else
    wasPlaying = false;

  t_size itemFocused = pm->playlist_get_focus_item(playlist);

  // insert position priority: after last selected, after focus, after currently playing
  // otherwise append

  if (lastselected != ~0) {
    position = lastselected;
  } else if (itemFocused != ~0) {
    position = itemFocused;
  } else if (wasPlaying) {
    position = itemPlaying;
  } else  // not playing, no selection and no item focused
    position = prevLength - 1;

  bool b_ADD = BlockFlagEnabled(flag, ACT_ADD);
  bool b_INS = BlockFlagEnabled(flag, ACT_INSERT);
  bool b_end_position = (position == prevLength - 1);

  // updateMask
  if (!b_INS && !b_ADD) {
    // REPLACE
    position = -1;
    pm->playlist_clear(playlist);
    updateMask.resize(tracks.get_count());
    for (int i = 0; i < tracks.get_count(); i++) updateMask.set(i, true);
  } else {
    if ((!b_INS && b_ADD) || (b_INS && b_end_position)) {
      // ADD
      position = prevLength - 1;
      for (int i = prevLength; i <= position + tracks.get_count(); i++)
        updateMask.set(i, true);
    } else {
      if (b_INS && !b_ADD) {
        // INSERT
        int pending = tracks.get_size();
        for (int i = 0; i < updateMask.size(); i++) {
          if (i <= position || i >= position + tracks.get_count() + pending)
            updateMask.set(i, false);
          else {
            updateMask.set(i, true);
            pending--;
          }
          if (i == position)
            pending++;
        }
      } else {
        if (b_INS && b_ADD)
          ;  // not implemented
      }
    }
  }
  // end updateMask

  // rating
  bool b_rated = BlockFlagEnabled(flag, ACT_RATED);
  bool b_best = BlockFlagEnabled(flag, ACT_RATED_HIGHT);

  if (b_rated || b_best) {
    pfc::list_t<metadb_handle_ptr> ratedTracks;
    int count_rated = GetOnlyRated(b_best, updateTracks, ratedTracks, updateMask);
    if (count_rated != updateTracks.get_count()) {
      updateTracks = ratedTracks;
    };
  }

  // backup and send (after previous item position)
  t_size t_pos = position + 1;
  t_size t_res = pfc_infinite;
  if (updateTracks.get_count() > 0) {
    pm->playlist_undo_backup(playlist);
    t_res = pm->playlist_insert_items(playlist, t_pos, updateTracks, bit_array_false());
  }
  return t_res;
}

int CustomAction::GetOnlyRated(bool onlyhighrates,
                               const pfc::list_base_const_t<metadb_handle_ptr>& tracks,
                               pfc::list_t<metadb_handle_ptr>& rated_items,
                               bit_array_bittable& updateMask) {

  int pending = tracks.get_size();
  int masksize = updateMask.size();
  bit_array_bittable ratedMask(masksize);
  int count_unrated = 0;
  for (t_size walk = updateMask.find_first(true, 0, masksize); walk < masksize;
       walk = updateMask.find_next(true, walk, masksize)) {
    metadb_handle_ptr atrack = tracks.get_item(tracks.get_size() - pending);
    pending--;
    if (CheckRate(atrack, onlyhighrates) > -1) {
      ratedMask.set(walk - count_unrated, true);
      rated_items.add_item(atrack);
    } else {
      count_unrated++;
    }
  }
  ratedMask.resize(masksize - count_unrated);
  updateMask.resize(ratedMask.size());
  for (int i = 0; i < updateMask.size(); i++)
    updateMask.set(i, ratedMask.get(i));

  return rated_items.get_count();
}

// track rates
// return values: 0 rated track, -1 not rated or
// track rate or -1 if hrate enabled
int CustomAction::CheckRate(metadb_handle_ptr track, bool hrate) {
  int res;
  service_ptr_t<titleformat_object> script;
  pfc::string8 format = "%rating%";

  if (static_api_ptr_t<titleformat_compiler>()->compile(script, format)) {
    static_api_ptr_t<playback_control> pbc;
    pbc->playback_format_title_ex(
        track, nullptr, format, script, nullptr, playback_control::display_level_titles);
    if (stricmp_utf8(format.get_ptr(), "?") != 0) {
      if (hrate) {
        try {
          res = std::stoi(format.get_ptr());
          res = (res >= configData->CustomActionHRate ? res : -1);
        } catch (...) {
          res = -1;
        }
      } else
        res = 0; // track is rated
    } else {
      res = -1;
    }
  } else
    res = -1;
  return res;
}
//... send to playlist
bool CustomAction::SendToPlaylist(const pfc::list_base_const_t<metadb_handle_ptr>& tracks,
                                  t_size playlist, int flag) {
  if (playlist == pfc_infinite)
    return false;

  static_api_ptr_t<playlist_manager> pm;

  //get prev selection
  t_size prevLength = pm->playlist_get_item_count(playlist);
  bit_array_bittable prevSelectionMask(prevLength);
  pm->playlist_get_selection_mask(playlist, prevSelectionMask);
  t_size lastSelected = ~0;
  for (t_size walk = prevSelectionMask.find_first(true, 0, prevLength);
       walk < prevLength;
       walk = prevSelectionMask.find_next(true, walk, prevLength)) {
    lastSelected = walk;
  }

  pfc::list_t<metadb_handle_ptr> updateTracks;
  bit_array_bittable updateMask(prevLength + tracks.get_count());

  //add insert replace & check rates
  t_size changeposition =
      DoAddReplace(tracks, playlist, flag, lastSelected, updateTracks, updateMask);

  if (BlockFlagEnabled(flag, ACT_ACTIVATE)) {
    if (changeposition != pfc_infinite) {
      pm->set_active_playlist(playlist);
      pm->playlist_ensure_visible(playlist, changeposition);
      pm->set_playing_playlist(playlist);
    }
  }

  if (BlockFlagEnabled(flag, ACT_PLAY)) {
    if (changeposition != pfc_infinite) {
      DoPlayItem(0, playlist, updateMask, flag);
    }
  }

  if (BlockFlagEnabled(flag, ACT_HIGHLIGHT)) {
    if (changeposition != pfc_infinite) {
      // set bit_array_false to OR selection with updateMask
      // set bit_array_true to AND updateMask with selection
      pm->playlist_set_selection(playlist, bit_array_true(), updateMask);
    }
  }
  return (changeposition != pfc_infinite);
}

///////////////////////////////////////////////////////////////////////////////////////

namespace {

class TargetActivePlaylist : public CustomAction {
 public:
  TargetActivePlaylist() : CustomAction("Send to Active Playlist ", true) {}
  void sendconfig(HWND) {};
  void run(const pfc::list_base_const_t<metadb_handle_ptr>& tracks,
           const char* /*albumTitle*/, HWND hwnd, int flag) override {

    static_api_ptr_t<playlist_manager> pm;
    t_size playlist = pm->get_active_playlist();

    // check exists, locked or conflicts with source playlist mode
    if (!availablePlaylist(playlist)) {
      NotifyRejected(hwnd);
      return;
    }

    bool bdone = SendToPlaylist(tracks, playlist, flag);

    if (!bdone)
      NotifyRejected(hwnd);
  }
};

class TargetDefaultPlaylist : public CustomAction {
 public:
  TargetDefaultPlaylist() : CustomAction("Send to Default Playlist ", true) {}
  void sendconfig(HWND) {};
  void run(const pfc::list_base_const_t<metadb_handle_ptr>& tracks,
           const char* /*albumTitle*/, HWND hwnd, int flag) override {

    static_api_ptr_t<playlist_manager> pm;
    t_size playlist = pm->find_playlist(configData->TargetPlaylist);

    if (playlist == ~0) {
      // create playlist
      playlist = pm->create_playlist(configData->TargetPlaylist, ~0u, ~0u);
    } else {
      bool b_locked = playlist_manager::get()->playlist_lock_is_present(playlist);
      if (b_locked) {
        NotifyRejected(hwnd);
        return;
      }
    }

    bool bdone = SendToPlaylist(tracks, playlist, flag);

    if (!bdone)
      NotifyRejected(hwnd);
  }
};

class NewPlaylist : public CustomAction {
 public:
  NewPlaylist() : CustomAction("Send to Album Playlist ", true) {}
  void sendconfig(HWND){};
  void run(const pfc::list_base_const_t<metadb_handle_ptr>& tracks,
           const char* albumTitle, HWND hwnd, int flag) override {
    static_api_ptr_t<playlist_manager> pm;
    t_size playlist = pm->find_playlist(albumTitle);

    if (playlist == ~0) {
      // create playlist
      playlist = pm->create_playlist(albumTitle, ~0u, ~0u);
    } else {
      bool b_locked = playlist_manager::get()->playlist_lock_is_present(playlist);
      if (b_locked) {
        NotifyRejected(hwnd);
        return;
      }
    }

    bool bdone = SendToPlaylist(tracks, playlist, flag);

    if (!bdone)
      NotifyRejected(hwnd);
  }
};

//pre:   -1  action activates visualization defined in post parameter
//      >-1  action will toggle visualization pre and post
//post:  -1  sets default visualization

class ActionDisplayConfig : public CustomAction {
 public:
  // Action name "Set Display" or "Toggle Display" + config indexes
  ActionDisplayConfig(int pre, int post) : CustomAction("", false), m_pre(pre), m_post(post) {

    const pfc::string8 str_set_display_action = "Set Display";
    const pfc::string8 str_toggle_display_action = "Toggle Display";

    if (pre == -1) {
      actionName = str_set_display_action.c_str(); //"Set Display Ndx"
      actionName = actionName << pfc::string8(" ")
                              << (post == -1 ? "to Default" : std::to_string(post).c_str());
    }
    else {
      actionName = str_toggle_display_action.c_str(); //"Toggle Display 1..Ndx or 1..default"
      actionName = actionName << pfc::string8(" ") << pre;
      actionName = actionName << ".." << (post == -1? "default" : std::to_string(post).c_str());
    }
    bPlaylistAction = false;
    m_pre = pre;
    m_post = post;
  }

  void run(const pfc::list_base_const_t<metadb_handle_ptr>& tracks,
    const char* /*albumTitle*/, HWND hwnd, int flag) {};
  void sendconfig(HWND hwnd) override {
    int currentpos = configData->sessionCompiledCPInfo.get().first;
    int defaultpos = configData->GetCCPosition();
    int ndx_pre = m_pre; // -1 for set only action, other for swap  action pre and post
    int ndx_post = m_post == -1 ? defaultpos : m_post;
    // set to post or toggle to post
    if ((m_pre == -1 && currentpos != ndx_post) ||
        (m_pre !=-1 && currentpos == ndx_pre) ||
        (m_pre !=-1 && currentpos != ndx_pre && currentpos != ndx_post)) {
      ndx_pre = currentpos != ndx_pre ? currentpos : ndx_pre;
      if (currentpos != ndx_post)
        SendMessage(hwnd, WM_MYACTIONS_SET_DISPLAY, ndx_pre, ndx_post);
    }
    else {
      //toggle to pre
      if (currentpos != ndx_pre)
        SendMessage(hwnd, WM_MYACTIONS_SET_DISPLAY, currentpos, ndx_pre);
    }
  }

 private:
  int m_pre;
  int m_post;
};

}  // namespace

std::vector<CustomAction*> g_customActions{
    new TargetActivePlaylist, new TargetDefaultPlaylist, new NewPlaylist,
    new ActionDisplayConfig(-1,1), new ActionDisplayConfig(0, -1)};

void executeAction(const char* action, const ::db::AlbumInfo& album, HWND hwnd,
                   int flag) {

  for (auto& g_customAction : g_customActions) {
    if (stricmp_utf8(action, g_customAction->actionName) == 0) {
      if (g_customAction->isPlaylistAction())
        g_customAction->run(album.tracks, album.title.c_str(), hwnd, flag);
      else
        g_customAction->sendconfig(hwnd);
      return;
    }
  }
  GUID commandGuid;
  if (menu_helpers::find_command_by_name(action, commandGuid)) {
    menu_helpers::run_command_context(commandGuid, pfc::guid_null, album.tracks);
  }
}
