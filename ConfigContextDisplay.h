#pragma once
#include "ConfigData.h"
#include "EngineWindow.h"

namespace coverflow {

  void AppendDisplayContextMenuOptions(HMENU* hMenu) {
    int maxactionshortcuts = 10 - 1;
    CoverConfigMap coverconfigs = configData->CoverConfigs;

    int session_pos = configData->sessionCompiledCPInfo.get().first;
    auto& [session_name, session_config] = *std::next(coverconfigs.begin(), session_pos);
    int default_pos = configData->GetCCPosition();
    int tot_displays = coverconfigs.size();

    HMENU _childDisplay = CreatePopupMenu();

    uAppendMenu(_childDisplay, MF_STRING | session_pos == default_pos ? MF_DISABLED : MF_ENABLED,
                engine::ID_DISPLAY_0, PFC_string_formatter() << configData->CoverConfigSel.c_str() << " (default)"
                << "\t Ctrl + 0");

    int ndx = 0;
    for (auto& [name, config] : configData->CoverConfigs) {
      uAppendMenu(_childDisplay, MF_STRING | ndx == session_pos ? MF_CHECKED | MF_DISABLED : MF_UNCHECKED,
                  engine::ID_DISPLAY_0 + ndx + 1, PFC_string_formatter() << name.c_str()
                  << "\t Ctrl + " << pfc::toString(ndx+1).c_str());
      ndx++;
      if (ndx >= maxactionshortcuts) break;
    }

    MENUITEMINFO mi1 = { 0 };
    mi1.cbSize = sizeof(MENUITEMINFO);
    mi1.fMask = MIIM_SUBMENU | MIIM_STRING | MIIM_ID;
    mi1.wID = engine::ID_SUBMENU_DISPLAY;
    mi1.hSubMenu = _childDisplay;
    mi1.dwTypeData = _T("Display");

    InsertMenuItem(*hMenu, engine::ID_CONTEXT_LAST-10, true, &mi1);
  }
  void OnDisplayContextCommand(HMENU* hMenu, const int cmd, engine::EngineWindow* ew) {
    int ndx = cmd - engine::ID_DISPLAY_0 - 1;
    int default_pos = configData->GetCCPosition();
    ndx = ndx == -1? default_pos: ndx;
    ew->cmdActivateVisualization(ndx);
  }
} // namespace coverflow
