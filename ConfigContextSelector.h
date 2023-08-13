#pragma once
#include "ConfigData.h"
#include "EngineWindow.h"

namespace coverflow {

  void AppendSelectorContextMenuOptions(HMENU* hMenu, bool bonlylibseltoggle, engine::EngineWindow* ew) {

    bool b_iswholelib = ew->container.coverIsWholeLibrary();
    bool b_lfs_enabled = configData->SourceLibrarySelector;
    bool b_lfs_lock_enabled = configData->SourceLibrarySelectorLock;
    bool b_xui_follow_lib_sel = ew->container.GetCoverDispFlagU(DispFlags::FOLLOW_LIB_SEL);
    bool b_xui_sets_selection = ew->container.GetCoverDispFlagU(DispFlags::SET_LIB_SEL);

    if (b_iswholelib) {

      HMENU _childLibrarySelections = CreatePopupMenu();

      uAppendMenu(_childLibrarySelections, MF_STRING | b_xui_sets_selection ? MF_CHECKED : MF_UNCHECKED,
        engine::ID_COVER_SETS_SELECTION,
        PFC_string_formatter() << "Set Library Selection";
      uAppendMenu(_childLibrarySelections, MF_STRING | b_xui_follow_lib_sel ? MF_CHECKED : MF_UNCHECKED,
        engine::ID_LIBRARY_COVER_FOLLOWS_SELECTION,
        PFC_string_formatter() << "Follow Library Selection");


      MENUITEMINFO mi = {0};
      mi.cbSize = sizeof(MENUITEMINFO);
      mi.fMask = MIIM_SUBMENU | MIIM_STRING | MIIM_ID;
      mi.wID = engine::ID_SUBMENU_SELECTOR;
      mi.hSubMenu = _childLibrarySelections;
      mi.dwTypeData = (LPWSTR)kmenu_label_LibrarySelectionCovers;
      
      InsertMenuItem(*hMenu, engine::ID_CONTEXT_LAST_DISPLAY - 20, true, &mi);
    }
  }

} // namespace coverflow

