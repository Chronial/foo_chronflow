#pragma once
#include "ConfigData.h"
#include "EngineWindow.h"

namespace coverflow {

  void AppendSelectorContextMenuOptions(HMENU* hMenu) {
    bool b_iswholelib = configData->IsWholeLibrary();
    //follow library
    bool b_followlibrary = configData->CoverFollowsLibrarySelection;
    //filter
    bool b_lfs_enabled = configData->SourceLibrarySelector;
    bool b_lfs_lock_enabled = configData->SourceLibrarySelectorLock;

    HMENU _childDisplay = CreatePopupMenu();
    if (b_iswholelib)
    uAppendMenu(*hMenu, !b_iswholelib? MF_DELETE : MF_STRING | b_followlibrary ? MF_CHECKED : MF_UNCHECKED,
      engine::ID_LIBRARY_COVER_FOLLOWS_SELECTION,
      PFC_string_formatter() << "Covers follow Library Selection" << "\t");

    uAppendMenu(_childDisplay, MF_STRING | !b_followlibrary && !b_lfs_enabled? MF_DISABLED : b_lfs_enabled ? MF_CHECKED : MF_UNCHECKED,
      engine::ID_LIBRARY_FILTER_SELECTOR_AS_SOURCE,
      PFC_string_formatter() << "Library Filter Selector"
      << "\tCtrl+F10");

    uAppendMenu(_childDisplay, MF_STRING | !b_lfs_enabled && !b_lfs_lock_enabled? MF_DISABLED | MF_UNCHECKED : b_lfs_lock_enabled ? MF_CHECKED : MF_UNCHECKED,
      engine::ID_LIBRARY_FILTER_SELECTOR_LOCK,
      PFC_string_formatter() << "Lock Library Filter Selector"
      << "\tCtrl+F8");

    MENUITEMINFO mi1 = { 0 };
    mi1.cbSize = sizeof(MENUITEMINFO);
    mi1.fMask = MIIM_SUBMENU | MIIM_STRING | MIIM_ID;
    mi1.wID = engine::ID_SUBMENU_SELECTOR;
    mi1.hSubMenu = _childDisplay;
    mi1.dwTypeData = _T("Select Covers");
    // insert menu item with submenu to hPopupMenu
    InsertMenuItem(*hMenu, engine::ID_CONTEXT_LAST - 15, true, &mi1);
  }
  void OnSelectorContextCommand(HMENU* hMenu, const int cmd, engine::EngineWindow* ew) {

    if (cmd == engine::ID_LIBRARY_COVER_FOLLOWS_SELECTION) {
      //
      // toggle covers follow library selection
      ew->cmdToggleLibraryCoverFollowsSelection();
    }
    else if (cmd == engine::ID_LIBRARY_FILTER_SELECTOR_AS_SOURCE) {
      //
      // toggle Library Filter Selector on/off
      ew->cmdToggleLibraryFilterSelectorSource(false);
    }
    else if (cmd == engine::ID_LIBRARY_FILTER_SELECTOR_LOCK) {
      //
      // toggle Library Filter Selector on/off
      ew->cmdToggleLibraryFilterSelectorSource(true);
    }

  }

} // namespace coverflow

