#pragma once
#include "ConfigData.h"
#include "EngineWindow.h"

namespace coverflow {

auto kmenu_label_PlayListCovers = _T("Playlist Covers");
auto kmenu_label_Display = _T("Display");
auto kmenu_label_SelectCovers = _T("Manual Selector");
auto kmenu_label_LibrarySelectionCovers = _T("Library selections");
auto kmenu_label_PlaylistSelectionCovers = _T("Playlist selections");

void AppendPlaylistContextMenuOptions(HMENU* hMenu, engine::EngineWindow* ew) {
  if (configData->CtxShowPlaylistMenu) {

    pfc::string8 plname;
    pfc::string8 plguid;

    static_api_ptr_t<playlist_manager_v5> pm;
    pm->activeplaylist_get_name(plname);
    GUID guid = pm->playlist_get_guid(pm->get_active_playlist());
    plguid = pfc::print_guid(guid);

    bool b_source = pfc::guid_equal(pfc::GUID_from_text(ew->container.GetSourcePlaylistGUID()), guid);

    bool b_setplaylistsel = ew->container.GetCoverDispFlagU(DispFlags::SET_PL_SEL);
    bool b_followplaylist = ew->container.GetCoverDispFlagU(DispFlags::FOLLOW_PL_SEL);
    bool b_playlistsource_valid = !ew->container.GetSourcePlaylistName().equals("");

    bool b_iswholelib = ew->container.coverIsWholeLibrary();
    bool b_activesource_enabled = ew->container.GetCoverDispFlagU(DispFlags::SRC_ACTPLAYLIST);
    bool b_playlistsource_enabled = !b_activesource_enabled && ew->container.GetCoverDispFlagU(DispFlags::SRC_PLAYLIST);

    bool b_playlistsource_grouped = !ew->container.GetCoverDispFlagU(DispFlags::SRC_PL_UNGROUPED);
    bool b_playlistsource_hilight = ew->container.GetCoverDispFlagU(DispFlags::SRC_PL_HL);

    HMENU _childPlaylist = CreatePopupMenu();
    HMENU _childSelections = CreatePopupMenu();

    if (!b_iswholelib) {
      uAppendMenu(_childSelections, MF_STRING | (b_setplaylistsel ? MF_CHECKED : MF_UNCHECKED),
                    engine::ID_PLAYLIST_SET_PL_SELECTION,
                    PFC_string_formatter() << "Set Playlist Track Selection");
      uAppendMenu(_childSelections, MF_STRING | (b_followplaylist ? MF_CHECKED : MF_UNCHECKED),
                  engine::ID_PLAYLIST_FOLLOWS_PL_SELECTION,
                  PFC_string_formatter() << "Follow Playlist Track Selection");

      MENUITEMINFO mi0 = {0};
      mi0.cbSize = sizeof(MENUITEMINFO);
      mi0.fMask = MIIM_SUBMENU | MIIM_STRING | MIIM_ID;
      mi0.wID = engine::ID_SUBMENU_LIBRARY_SELECTIONS;
      mi0.hSubMenu = _childSelections;
      mi0.dwTypeData = (LPWSTR)kmenu_label_PlaylistSelectionCovers;

      InsertMenuItem(*hMenu, engine::ID_CONTEXT_LAST_DISPLAY - 10, true, &mi0);
    }

    //F9 toggle Playlist as Source
    uAppendMenu(_childPlaylist,
                MF_STRING | (!b_playlistsource_valid ? MF_DISABLED
        : b_playlistsource_enabled ? MF_CHECKED : MF_UNCHECKED),
        engine::ID_PLAYLIST_CURRENT_AS_SOURCE,
                PFC_string_formatter()
                    << "Source Playlist: " << ew->container.GetSourcePlaylistName()
        << "\tF9");
    //F8 assign Playlist name and activate
    uAppendMenu(_childPlaylist, MF_STRING | (b_source ? MF_DISABLED : MF_ENABLED),
        engine::ID_PLAYLIST_SOURCE_SET,
        PFC_string_formatter() << "Set Current Playlist as Source"
        << "\tF8");
    //F10 active playlist as Source
    uAppendMenu(_childPlaylist, MF_STRING | (b_activesource_enabled ? MF_CHECKED : MF_UNCHECKED),
        engine::ID_PLAYLIST_ACTIVE_AS_SOURCE,
        PFC_string_formatter() << "Active Playlist as Source"
        << "\tF10");
    if (!b_iswholelib) {
      uAppendMenu(_childPlaylist, MF_SEPARATOR, 0, nullptr);
      uAppendMenu(_childPlaylist,
                  MF_STRING | (!b_playlistsource_grouped ? MF_CHECKED : MF_UNCHECKED),
                  engine::ID_PLAYLIST_GROUPED,
                  PFC_string_formatter() << "Show playlist tracks"
                                         << "\tF4");
      uAppendMenu(_childPlaylist,
                  MF_STRING | (b_setplaylistsel ? 0 : MF_DISABLED) | (b_playlistsource_hilight ? MF_CHECKED : MF_UNCHECKED),
                  engine::ID_PLAYLIST_HILIGHT,
                  PFC_string_formatter() << "Highlight cover tracks" << (!b_setplaylistsel ? " (req. Selections >Track Selections)": "" ));
    }
    MENUITEMINFO mi1 = { 0 };
    mi1.cbSize = sizeof(MENUITEMINFO);
    mi1.fMask = MIIM_SUBMENU | MIIM_STRING | MIIM_ID;
    mi1.wID = engine::ID_SUBMENU_PLAYLIST;
    mi1.hSubMenu = _childPlaylist;
    mi1.dwTypeData = (LPWSTR)kmenu_label_PlayListCovers;

    InsertMenuItem(*hMenu, engine::ID_CONTEXT_LAST_DISPLAY - 5, true, &mi1);
  }
}

void OnPlaylistContextCommand(HMENU* hMenu, const int cmd, engine::EngineWindow* ew) {
  if (cmd == engine::ID_PLAYLIST_SET_PL_SELECTION) {
    // toggle cover follows selecton
    ew->container.ToggleCoverDispFlagU(DispFlags::SET_PL_SEL);
  } else if (cmd == engine::ID_PLAYLIST_FOLLOWS_PL_SELECTION) {
    // toggle cover follows selecton
    ew->container.ToggleCoverDispFlagU(DispFlags::FOLLOW_PL_SEL);
  } else if (cmd == engine::ID_PLAYLIST_CURRENT_AS_SOURCE) {
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
  } else if (cmd == engine::ID_PLAYLIST_GROUPED) {
    //F4
    // toggle grouped
    ew->cmdTogglePlaylistGrouped();
  } else if (cmd == engine::ID_PLAYLIST_HILIGHT) {
    //
    //  toggle hilight playlist content
    ew->cmdTogglePlaylistHiLight();
  }
}

} // namespace coverflow
