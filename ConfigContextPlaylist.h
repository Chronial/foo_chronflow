#pragma once
#include "ConfigData.h"
#include "EngineWindow.h"

namespace coverflow {

void AppendPlaylistContextMenuOptions(HMENU* hMenu) {
  if (!configData->CtxHidePlaylistMenu) {

    pfc::string8 plname;
    static_api_ptr_t<playlist_manager> pm;
    pm->activeplaylist_get_name(plname);

    bool b_iswholelib = configData->IsWholeLibrary();
    bool b_followplaylist = configData->CoverFollowsPlaylistSelection;
    bool b_playlistsource_valid = !configData->SourcePlaylistName.equals("");
    bool b_activesource_enabled = configData->SourceActivePlaylist;
    bool b_playlistsource_enabled = !configData->SourceActivePlaylist && configData->SourcePlaylist;
    bool b_sourcetarget = ((stricmp_utf8(configData->SourcePlaylistName, plname) == 0) ||
        (stricmp_utf8(configData->TargetPlaylist, plname) == 0));

    HMENU _childPlaylist = CreatePopupMenu();
    if (!b_iswholelib)
    uAppendMenu(*hMenu, MF_STRING | b_followplaylist ? MF_CHECKED : MF_UNCHECKED,
                engine::ID_PLAYLIST_FOLLOWS_PL_SELECTION,
                PFC_string_formatter() << "Covers follow Playlist Selection"
                                       << "\t");
    //F9 toggle Playlist as Source
    uAppendMenu(_childPlaylist, MF_STRING | !b_playlistsource_valid ? MF_DISABLED
        : b_playlistsource_enabled ? MF_CHECKED : MF_UNCHECKED,
        engine::ID_PLAYLIST_CURRENT_AS_SOURCE,
        PFC_string_formatter() << "Source Playlist: " << configData->SourcePlaylistName
        << "\tF9");
    //F8 assign Playlist name and activate
    uAppendMenu(_childPlaylist, MF_STRING | b_sourcetarget ? MF_DISABLED : MF_ENABLED,
        engine::ID_PLAYLIST_SOURCE_SET,
        PFC_string_formatter() << "Set Current Playlist as Source"
        << "\tF8");
    //F10 active playlist as Source
    uAppendMenu(_childPlaylist, MF_STRING | b_activesource_enabled ? MF_CHECKED : MF_UNCHECKED,
        engine::ID_PLAYLIST_ACTIVE_AS_SOURCE,
        PFC_string_formatter() << "Active Playlist as Source"
        << "\tF10");
    MENUITEMINFO mi1 = { 0 };
    mi1.cbSize = sizeof(MENUITEMINFO);
    mi1.fMask = MIIM_SUBMENU | MIIM_STRING | MIIM_ID;
    mi1.wID = engine::ID_SUBMENU_PLAYLIST;
    mi1.hSubMenu = _childPlaylist;
    mi1.dwTypeData = _T("PlayList Covers");
    // insert menu item with submenu to hPopupMenu
    InsertMenuItem(*hMenu, engine::ID_CONTEXT_LAST - 5, true, &mi1);
  }
}
void OnPlaylistContextCommand(HMENU* hMenu, const int cmd, engine::EngineWindow* ew) {
  if (cmd == engine::ID_PLAYLIST_FOLLOWS_PL_SELECTION) {
    // toggle cover follows selecton
    configData->CoverFollowsPlaylistSelection = !configData->CoverFollowsPlaylistSelection;
  }
  if (cmd == engine::ID_PLAYLIST_CURRENT_AS_SOURCE) {
    //F9
    //toggle playlist source on/off
    ew->cmdTogglePlaylistSource();
  } else if (cmd == engine::ID_PLAYLIST_SOURCE_SET) {
    //F8
    //activate and assign current playlist as source
    ew->cmdAssignPlaylistSource();
  } else if (cmd == engine::ID_PLAYLIST_ACTIVE_AS_SOURCE) {
    //F10
    // toggle active playlist is the source on/off
    ew->cmdToggleActivePlaylistSource();
  }

}

} // namespace coverflow
