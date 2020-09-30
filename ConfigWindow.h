#pragma once
// clang-format off
#include <windowsx.h>

#include <boost/range/iterator_range.hpp>

#include "./lib/win32_helpers.h"
#include "ConfigBindings.h"
#include "ConfigData.h"
#include "Engine.h"
#include "EngineThread.h"
#include "MyActions.h"
#include "cover_positions_compiler.h"
#include "doc/key_config.h"
#include "helpers/win32_dialog.h"
#include "resource.h"
#include "winuser.h"

//need default_SourcePlaylistName
#include "configData.h"

//#include "ConfigDialogDisplay.h"
// clang-format on

namespace coverflow {
extern char const* const default_SourcePlaylistName;
using ::engine::EngineThread;
using EM = ::engine::Engine::Messages;

extern std::unordered_multimap<UINT, int> disableMap;
extern std::unordered_multimap<UINT, int> disableStateMatch;

//data binding notifications to parent dialog
const UINT CF_USER_CONFIG_NEWTAB(WM_USER + 0x7776);
const UINT CF_USER_CONFIG_CHANGE(WM_USER + 0x7778);

/**************************************************************************************/
/**************************************************************************************/

class ConfigTab {
 protected:
  UINT id;
  HWND parent;
  char* title;
  HWND hWnd = nullptr;
  RECT m_rcTab;
  //testing candidate to new dlg schema
  //CConfigTabCtrl m_ConfigTabCtrl;
  //CDialogUIElem m_dlg;

 public:

  ConfigTab(char* title, UINT id, HWND parent) : id(id), parent(parent), title(title) {
    HWND hWndTab = uGetDlgItem(parent, IDC_TABS);
    uTCITEM tabItem = {0};
    tabItem.mask = TCIF_TEXT;
    tabItem.pszText = title;
    int idx = TabCtrl_GetItemCount(hWndTab);
    uTabCtrl_InsertItem(hWndTab, idx, &tabItem);
    GetWindowRect(hWndTab, &m_rcTab);

    uSendMessage(hWndTab, TCM_ADJUSTRECT, FALSE, reinterpret_cast<LPARAM>(&m_rcTab));

    MapWindowPoints(HWND_DESKTOP, parent, (POINT*)&m_rcTab, 2);
    // Resize the tree control so that it fills the tab control.
    InflateRect(&m_rcTab, 2, 1);
    OffsetRect(&m_rcTab, -1, 1);
  }

  NO_MOVE_NO_COPY(ConfigTab);

  HWND getTabWnd() {
    return hWnd;
  }
  void SetTabWnd(HWND hwnd) {
    if (hWnd) DestroyWindow(hWnd);
    hWnd = hwnd;
    return ;
  }
  virtual ~ConfigTab() {
    if (hWnd != nullptr)
      DestroyWindow(hWnd);
  }

  void hide() { ShowWindow(hWnd, SW_HIDE); }

  void show() {
    if (parent == nullptr) {
      return;
    } else {
      HWND hWndTab = uGetDlgItem(parent, IDC_TABS);
      RECT tabRect;
      GetWindowRect(hWndTab, &tabRect);
      hWnd = uCreateDialog(id, parent, dialogProxy, reinterpret_cast<LPARAM>(this));
      SetWindowPos(hWnd, nullptr, m_rcTab.left, m_rcTab.top, m_rcTab.right - m_rcTab.left,
        m_rcTab.bottom - m_rcTab.top, SWP_NOZORDER | SWP_NOACTIVATE);
      ShowWindow(hWnd, SW_SHOW);
    }
  }

  virtual BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam) = 0;

  static BOOL CALLBACK dialogProxy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    //FB2K_console_formatter() << "CALLBACK ConfigWindow::dialogProc()";

    ConfigTab* configTab = nullptr;

    if (uMsg == WM_INITDIALOG) {
      configTab = reinterpret_cast<ConfigTab*>(lParam);
      SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(configTab));
      configTab->hWnd = hWnd;
    }
    else {
      configTab = reinterpret_cast<ConfigTab*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (configTab == nullptr)
      return FALSE;

    return configTab->dialogProc(hWnd, uMsg, wParam, lParam);
  }

  void loadDisableMap() {
    for (auto [checkboxId, itemToDisable] : disableMap) {
      bool enabled = uButton_GetCheck(hWnd, checkboxId);
      if (itemToDisable < 0) {
        uEnableWindow(uGetDlgItem(hWnd, -itemToDisable), static_cast<BOOL>(!enabled));
      } else {
        uEnableWindow(uGetDlgItem(hWnd, itemToDisable), static_cast<BOOL>(enabled));
      }
    }
    for (auto [checkboxId, itemToDisable] : disableStateMatch) {
      bool disabled = !IsWindowEnabled(uGetDlgItem(hWnd, checkboxId));
      if (disabled)
        uEnableWindow(uGetDlgItem(hWnd, itemToDisable), FALSE);
    }
  }

  void notifyParent(const UINT ndxTab, const UINT code) {
    //CF_USER_CONFIG_NEWTAB
    //CF_USER_CONFIG_CHANGE

    NMHDR lparam;
    lparam.code = code;
    lparam.idFrom = ndxTab;
    lparam.hwndFrom = hWnd;

    uSendMessage(parent, WM_NOTIFY, (WPARAM)IDC_TABS, (LPARAM)&lparam);
  }

  void buttonClicked(UINT id) {
    loadDisableMap();
    notifyParent(id, CF_USER_CONFIG_CHANGE);
  }

  void textChanged(UINT id) {
    notifyParent(id, CF_USER_CONFIG_CHANGE);
  }

  void listSelChanged(UINT id) {
    int s = uSendDlgItemMessage(hWnd, id, CB_GETCURSEL, 0, 0);
    if (s != CB_ERR) {
      notifyParent(id, CF_USER_CONFIG_CHANGE);
    }
  }

 protected:
  void redrawMainWin() {
    EngineThread::forEach([](auto& t) { t.invalidateWindow(); });
  }
};

#define CONFIG_TAB(CLASS, TITLE, IDD) \
  CLASS(HWND parent) : ConfigTab(TITLE, IDD, parent) {}

#if 0
// minimal Tab setup
class SomeTab : public ConfigTab {
public:
	CONFIG_TAB(SomeTab, TITLE, IDD);
	BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		switch (uMsg)
		{
		case WM_INITDIALOG:
			loadConfig();
			break;
		case WM_COMMAND:
			if (HIWORD(wParam) == EN_CHANGE){
				textChanged(LOWORD(wParam));
			} else if (HIWORD(wParam) == BN_CLICKED) {
				buttonClicked(LOWORD(wParam));
			}
			break;
		}
		return FALSE;
	}
};
#endif
/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/
using ListMap = std::unordered_map<int, std::string>;
extern ListMap customActionAddReplaceMap;

class BehaviourTab : public ConfigTab {
 public:
  CONFIG_TAB(BehaviourTab, "Behaviour", IDD_BEHAVIOUR_TAB);
  const struct {
    int chkID;
    int chkBlock;
    uint32_t flag;
  } action_flags_checkbox[ncheckboxflag * nblock] = {
      // sort definitions by flag value/ndx within each block
      {IDC_CHECK_BEHA_ACTFLAG_D_PLAY, 0, ACT_PLAY},            // flag 0
      {IDC_CHECK_BEHA_ACTFLAG_D_SET, 0, ACT_ACTIVATE},         // flag 1
      {IDC_CHECK_BEHA_ACTFLAG_D_HIGHLIGHT, 0, ACT_HIGHLIGHT},  // flag 3
      {IDC_CHECK_BEHA_ACTFLAG_D_RATED, 0, ACT_RATED},          // flag 5
      {IDC_CHECK_BEHA_ACTFLAG_D_RATED_HIGHT, 0, ACT_RATED_HIGHT},  // flag 6
      {IDC_CHECK_BEHA_ACTFLAG_M_PLAY, 1, ACT_PLAY},            // flag 0
      {IDC_CHECK_BEHA_ACTFLAG_M_SET, 1, ACT_ACTIVATE},         // flag 1
      {IDC_CHECK_BEHA_ACTFLAG_M_HIGHLIGHT, 1, ACT_HIGHLIGHT},  // flag 3
      {IDC_CHECK_BEHA_ACTFLAG_M_RATED, 1, ACT_RATED},          // flag 5
      {IDC_CHECK_BEHA_ACTFLAG_M_RATED_HIGHT, 1, ACT_RATED_HIGHT},  // flag 6
      {IDC_CHECK_BEHA_ACTFLAG_E_PLAY, 2, ACT_PLAY},            // flag 0
      {IDC_CHECK_BEHA_ACTFLAG_E_SET, 2, ACT_ACTIVATE},         // flag 1
      {IDC_CHECK_BEHA_ACTFLAG_E_HIGHLIGHT, 2, ACT_HIGHLIGHT},  // flag 3
      {IDC_CHECK_BEHA_ACTFLAG_E_RATED, 2, ACT_RATED},          // flag 5
      {IDC_CHECK_BEHA_ACTFLAG_E_RATED_HIGHT, 2, ACT_RATED_HIGHT}  // flag 6
      // still place for another flag (8 in total)
  };
  
  UINT action_block_combo[nblock] = {IDC_DOUBLE_CLICK, IDC_MIDDLE_CLICK, IDC_ENTER_KEY};

  // note: another operation available (4 in total)
  UINT action_add_replace_combo[nblock] = {IDC_COMBO_BEHA_ADD_REPLACE_FLAG_D,
                                           IDC_COMBO_BEHA_ADD_REPLACE_FLAG_M,
                                           IDC_COMBO_BEHA_ADD_REPLACE_FLAG_E};
  void FlagHide(HWND hWnd, bool enable) {
    // ShowWindow(hwndCheckbox, enable ? SW_SHOW : SW_HIDE);
    EnableWindow(hWnd, enable);
  }

  int isActionComboBox(UINT uint) {
    std::vector<UINT> vec(
        action_block_combo,
        action_block_combo + sizeof(action_block_combo) / sizeof(action_block_combo[0]));
    std::vector<UINT>::iterator it = std::find(vec.begin(), vec.end(), uint);

    if (it != vec.end()) {
      return std::distance(vec.begin(), it);
    } else
      return -1;
  }
  int isAddReplaceComboBox(UINT uint) {
    std::vector<UINT> vec(
        action_add_replace_combo,
        action_add_replace_combo +
            sizeof(action_add_replace_combo) / sizeof(action_add_replace_combo[0]));
    std::vector<UINT>::iterator it = std::find(vec.begin(), vec.end(), uint);

    if (it != vec.end()) {
      return std::distance(vec.begin(), it);
    } else
      return -1;
  }

  int getCheckboxFlagType(UINT uint) {
    HWND hwndCheckbox = uGetDlgItem(hWnd, uint);
    return GetWindowLongPtr(hwndCheckbox, GWLP_USERDATA);
  }

  int getCheckboxFlagBlock(UINT uint) {
    int res = -1;
    for (int i = 0; i < sizeof(action_flags_checkbox) / sizeof(action_flags_checkbox[0]);
         i++)
      if (action_flags_checkbox[i].chkID == uint) {
        res = (int)i / ncheckboxflag;
        break;
      }
    return res;
  }

  int log2(int x) {
    int targetlevel = 0;
    while (x >>= 1) ++targetlevel;
    return targetlevel;
  }
  //todo: still unsure about these flags/reqs/UI  
  void InitFlagCheckBoxes(HWND hwnd, pfc::string8* blocklist, unsigned long initflags) {
    std::vector<std::vector<byte>> vallflags;
    ActionFlagsToArray(vallflags, initflags);

    // nflag(8) - ncheckbox (5) = excluded (3)
    const int add_ndx = log2(ACT_ADD);
    const int insert_ndx = log2(ACT_INSERT);
    const int todo_ndx = log2(ACT_TO_DO);

    // int excluded = 0;
    for (int itblock = 0; itblock < nblock; itblock++) {
      bool bshow = CustomAction::isCustomAction(g_customActions, blocklist[itblock], true);
      int excluded = 0;
      for (int itflag = 0; itflag < nflag; itflag++) {
        if (itflag == add_ndx || itflag == insert_ndx || itflag == todo_ndx) {
          excluded++;
        } else {
          UINT idcontrol =
              action_flags_checkbox[(itblock * ncheckboxflag) + itflag - excluded].chkID;
          // action_flags_checkbox[(itblock * nflag-1) + itflag - excluded].chkID;
          HWND hwndCheckbox = uGetDlgItem(hWnd, idcontrol);
          SetWindowLongPtr(
              hwndCheckbox, GWLP_USERDATA,
              action_flags_checkbox[(itblock * ncheckboxflag) + itflag - excluded].flag);
          // action_flags_checkbox[(itblock * (nflag-1)) + itflag - excluded].flag);
          if (vallflags[itblock][itflag] == 1)
            uButton_SetCheck(hWnd, idcontrol, true);
          FlagHide(hwndCheckbox, bshow);
        }
      }
    }
  }

  void InitFlagComboBoxes(HWND hwnd, pfc::string8* blocklist, unsigned long initflags) {
    std::vector<std::vector<byte>> vallflags;
    ActionFlagsToArray(vallflags, initflags);

    for (int itblock = 0; itblock < nblock; itblock++) {
      bool bshow = CustomAction::isCustomAction(g_customActions, blocklist[itblock], true);
      int selectedndx[nblock] = {0};
      UINT idcontrol = action_add_replace_combo[itblock];

      int add_ndx = log2(ACT_ADD);
      int insert_ndx = log2(ACT_INSERT);

      selectedndx[itblock] = vallflags[itblock][add_ndx] ? ACT_ADD : selectedndx[itblock];
      selectedndx[itblock] = vallflags[itblock][insert_ndx]
                                 ? selectedndx[itblock] | ACT_INSERT
                                 : selectedndx[itblock];
      // both add & insert not implemented yet
      if (vallflags[itblock][add_ndx] && vallflags[itblock][insert_ndx]) {
        selectedndx[itblock] = -1;  // should get here until implemented!!
      }

      pfc::string8 textSelection =
          customActionAddReplaceMap.at(selectedndx[itblock]).c_str();

      if (selectedndx[itblock] > -1) {
        uSendDlgItemMessageText(
            hWnd, idcontrol, CB_SELECTSTRING, 1, textSelection.c_str());
        HWND hwndCombo = uGetDlgItem(hWnd, idcontrol);
        FlagHide(hwndCombo, bshow);
      }
    }
  }

  BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/) final {
    switch (uMsg) {
      case WM_INITDIALOG: {
        // on tab initialization & reset button

        notifyParent(id, CF_USER_CONFIG_NEWTAB);

        SendDlgItemMessage(hWnd, IDC_FOLLOW_DELAY_SPINNER, UDM_SETRANGE32, 1, 999);
        SendDlgItemMessage(hWnd, IDC_SPIN_BEHA_HRATE, UDM_SETRANGE32, 1, 5);

        // config custom actions
        pfc::string8 blocklist[nblock] = {
            configData->DoubleClick,
            configData->MiddleClick,
            configData->EnterKey,
        };
        // ... combobox: fill list available actions & fix combo width
        for (int it = 0; it < nblock; it++) {
          loadActionList(action_block_combo[it], blocklist[it]);
          // fix combo dropped width
          HWND combo = GetDlgItem(hWnd, action_block_combo[it]);
          int width = SendMessage(combo, CB_GETDROPPEDWIDTH, 999, 999);
          SendMessage(combo, CB_SETDROPPEDWIDTH, width * 1.5, 0);
        }
        // ... combobox: fill list add/insert/replace
        for (int it = 0; it < nblock; it++)
          loadActionAddReplaceList(action_add_replace_combo[it], "" /*blocklist[it]*/);

        unsigned long currentFlags = std::stoull(
            uGetDlgItemText(hWnd, IDC_HIDDEN_CUSTOM_ACTION_FLAG).c_str(), nullptr, 0);

        // ... checkboxes: check/uncheck action flags
        InitFlagCheckBoxes(hWnd, blocklist, currentFlags);
        // ... checkboxes: action modifiers selected (add/insert/send)
        InitFlagComboBoxes(hWnd, blocklist, currentFlags);

        break;
      }
      case WM_COMMAND: {
        if (HIWORD(wParam) == EN_CHANGE) {
          // here also after flowtocontrol, calls to textchanged() are cheap before full init

          if (LOWORD(wParam) == IDC_FOLLOW_DELAY) {
            const int ival = std::clamp(
                int(uGetDlgItemInt(hWnd, IDC_FOLLOW_DELAY, nullptr, 1)), 1, 999);
            const int oldval = uGetDlgItemInt(hWnd, IDC_FOLLOW_DELAY, nullptr, 1);
            if (ival != oldval)
              uSetDlgItemInt(hWnd, IDC_FOLLOW_DELAY, ival, 1);
          }
          if (LOWORD(wParam) == IDC_EDIT_BEHA_HRATE) {
            const int ival = std::clamp(
                int(uGetDlgItemInt(hWnd, IDC_EDIT_BEHA_HRATE, nullptr, 1)), 1, 5);
            const int oldval = uGetDlgItemInt(hWnd, IDC_EDIT_BEHA_HRATE, nullptr, 1);
            if (ival != oldval)
              uSetDlgItemInt(hWnd, IDC_EDIT_BEHA_HRATE, ival, 1);
          }

          textChanged(LOWORD(wParam));

        } else if (HIWORD(wParam) == EN_UPDATE) {
          ;  // do nothing
        } else if (HIWORD(wParam) == BN_CLICKED) {

          buttonClicked(LOWORD(wParam));

          const int flagType = getCheckboxFlagType(LOWORD(wParam));
          if (flagType >= action_flags_checkbox[0].flag) {  // ACT_PLAY is the lowest value flag

            const int action_block = getCheckboxFlagBlock(LOWORD(wParam));

            const unsigned long currentFlags = std::stoull(
                uGetDlgItemText(hWnd, IDC_HIDDEN_CUSTOM_ACTION_FLAG).c_str(), nullptr, 0);

            const bool benable = (uButton_GetCheck(hWnd, LOWORD(wParam)));

            const unsigned long updatedflags = ActionFlagsCalculate(
                action_block, flagType, false, currentFlags, benable);

            uSetDlgItemText(
                hWnd, IDC_HIDDEN_CUSTOM_ACTION_FLAG, std::to_string(updatedflags).data());

            textChanged(IDC_HIDDEN_CUSTOM_ACTION_FLAG);
          }
        } else if (HIWORD(wParam) == CBN_SELCHANGE) {
          
          // check if an action selection combobox triggered the event

          if (const int block = isActionComboBox(LOWORD(wParam)); block > -1) {
            // selection change on custom action combobox
            listSelChanged(LOWORD(wParam));
            pfc::string8 selected;
            int ires = uSendDlgItemMessage(hWnd, LOWORD(wParam), CB_GETCURSEL, 0, 0);
            if (ires != CB_ERR) {
              uGetDlgItemText(hWnd, LOWORD(wParam), selected);
            } else {
              return FALSE;
            }

            // update flag checkboxes: enabled/disabled state for custom action
            
            bool bshow = CustomAction::isCustomAction(g_customActions, selected, true);
            for (int itflag = 0; itflag < ncheckboxflag; itflag++) {
              UINT idcontrol =
                  action_flags_checkbox[block * ncheckboxflag + itflag].chkID;
              HWND hwndCheckBox = uGetDlgItem(hWnd, idcontrol);
              FlagHide(hwndCheckBox, bshow);
            }
            // update flag combobox: enabled/disabled state for custom

            UINT idcontrol = action_add_replace_combo[block];
            HWND hwndControl = uGetDlgItem(hWnd, idcontrol);
            FlagHide(hwndControl, bshow);

          } else {

            // check if an add/replace selection combobox triggered the event

            if (int block = isAddReplaceComboBox(LOWORD(wParam)); block != -1) {
              int itemId = uSendDlgItemMessage(hWnd, LOWORD(wParam), CB_GETCURSEL, 0, 0);
              if (itemId == CB_ERR) {
                return FALSE;
              }
              int val =
                  uSendDlgItemMessage(hWnd, LOWORD(wParam), CB_GETITEMDATA, itemId, 0);

              unsigned long updatedflags = std::stoull(
                  uGetDlgItemText(hWnd, IDC_HIDDEN_CUSTOM_ACTION_FLAG).c_str(), nullptr,
                  0);
              updatedflags = ActionFlagsCalculate(
                  block, ACT_ADD, false, updatedflags, (val & ACT_ADD) == ACT_ADD);
              updatedflags = ActionFlagsCalculate(block, ACT_INSERT, false, updatedflags,
                                                  (val & ACT_INSERT) == ACT_INSERT);

              // will trigger textchanged(...)
              uSetDlgItemText(hWnd, IDC_HIDDEN_CUSTOM_ACTION_FLAG,
                              std::to_string(updatedflags).data());
            }
          }
        }
        break;
      }
    }
    return FALSE;
  }

  void loadActionList(UINT id, const char* selectedItem) {
    HWND list = GetDlgItem(hWnd, id);
    SetWindowRedraw(list, false);
    SendMessage(list, CB_RESETCONTENT, 0, 0);
    uSendMessageText(list, CB_ADDSTRING, 0, "");

    service_enum_t<contextmenu_item> menus;
    service_ptr_t<contextmenu_item> menu;
    while (menus.next(menu)) {
      pfc::string8_fastalloc menuPath;
      pfc::string8_fastalloc menuName;
      for (unsigned int i = 0; i < menu->get_num_items(); i++) {
        menu->get_item_default_path(i, menuPath);
        menu->get_item_name(i, menuName);
        if (!menuPath.is_empty())
          menuPath.add_string("/");
        if (strcmp(menuPath, "Play") == 0) {
          continue;
        }
        menuPath.add_string(menuName);
        uSendMessageText(list, CB_ADDSTRING, 0, menuPath);
      }
    }
    for (auto action : boost::adaptors::reverse(g_customActions)) {
      uSendMessageText(list, CB_INSERTSTRING, 0, action->actionName);
    }
    int itemId = uSendDlgItemMessageText(hWnd, id, CB_FINDSTRINGEXACT, 1, selectedItem);
    itemId = uSendDlgItemMessageText(hWnd, id, CB_SETCURSEL, itemId, 0); 
    SetWindowRedraw(list, true);
  }

  void loadActionAddReplaceList(UINT id, const char* selectedItem) {
    HWND list = GetDlgItem(hWnd, id);
    SetWindowRedraw(list, false);
    SendMessage(list, CB_RESETCONTENT, 0, 0);
    uSendMessageText(list, CB_ADDSTRING, 0, "");
    uSendDlgItemMessage(hWnd, id, CB_RESETCONTENT, 0, 0);
    for (auto& [val, text] : customActionAddReplaceMap) {
      int itemId = uSendDlgItemMessageText(hWnd, id, CB_ADDSTRING, 0, text.c_str());
      uSendDlgItemMessage(hWnd, id, CB_SETITEMDATA, itemId, val);
      if (stricmp_utf8(text.c_str(), selectedItem) == 0) {
        uSendDlgItemMessageText(hWnd, id, CB_SELECTSTRING, 1, selectedItem);
      }
    }
    SetWindowRedraw(list, true);
  }
};

/**************************************************************************************/
/**************************************************************************************/
class SourcesTab : public ConfigTab {
 public:
  CONFIG_TAB(SourcesTab, "Album Source", IDD_SOURCE_TAB);

  BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/) final {
    switch (uMsg) {
      case WM_INITDIALOG: {
        notifyParent(id, CF_USER_CONFIG_NEWTAB);
        loadComboSourcePlayList(IDC_COMBO_SOURCE_PLAYLIST_NAME,
                                configData->SourcePlaylistName);
        HWND combo = GetDlgItem(hWnd, IDC_COMBO_SOURCE_PLAYLIST_NAME);
        int width = SendMessage(combo, CB_GETDROPPEDWIDTH, 999, 999);
        SendMessage(combo, CB_SETDROPPEDWIDTH, width * 1.5, 0);
      } break;

      case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE) {
          textChanged(LOWORD(wParam));
        } else
        if (HIWORD(wParam) == CBN_SELCHANGE) {
          listSelChanged(LOWORD(wParam));
          pfc::string8 selected;
          int ires = uSendDlgItemMessage(hWnd, LOWORD(wParam), CB_GETCURSEL, 0, 0);
          if (ires != CB_ERR) {
            uGetDlgItemText(hWnd, LOWORD(wParam), selected);
          } else {
            return FALSE;
          }
          int itemId = uSendDlgItemMessage(hWnd, LOWORD(wParam), CB_GETCURSEL, 0, 0);
          if (itemId == CB_ERR) {
            return FALSE;
          }
          int val = uSendDlgItemMessage(hWnd, LOWORD(wParam), CB_GETITEMDATA, itemId, 0);
          val = val;
        } else
        if (HIWORD(wParam) == BN_CLICKED) {
          buttonClicked(LOWORD(wParam));
        }
        break;
    }
    return FALSE;
  }

  void loadComboSourcePlayList(UINT id, const char* selectedItem) {
    std::map<t_size, pfc::string8> playlistmap;
    
    bool bdefault_playlist_exists =
        (pfc_infinite != playlist_manager::get()->find_playlist(default_SourcePlaylistName));
    int DefId;

    HWND list = GetDlgItem(hWnd, id);
    SetWindowRedraw(list, false);
    SendMessage(list, CB_RESETCONTENT, 0, 0);
    uSendMessageText(list, CB_ADDSTRING, 0, "");
    uSendDlgItemMessage(hWnd, id, CB_RESETCONTENT, 0, 0);

    if (!bdefault_playlist_exists)
      DefId = uSendDlgItemMessageText(hWnd, id, CB_ADDSTRING, 0, default_SourcePlaylistName);

    t_size cplaylist = playlist_manager::get()->get_playlist_count();
    for (int i=0;i<cplaylist;i++) {
      pfc::string8 aname;
      t_size pl = playlist_manager::get()->playlist_get_name(i, aname);
      int rowId = uSendDlgItemMessageText(hWnd, id, CB_ADDSTRING, 0, aname);
      if (stricmp_utf8(default_SourcePlaylistName, aname) == 0) {
        DefId = rowId;
      }
      uSendDlgItemMessage(hWnd, id, CB_SETITEMDATA, rowId, i);
      if (uSendDlgItemMessage(hWnd, id, CB_GETCURSEL, 0, 0) == CB_ERR) {
        if (stricmp_utf8(default_SourcePlaylistName, selectedItem) == 0) {
          uSendDlgItemMessageText(hWnd, id, CB_SETCURSEL, DefId, 0);
        } else
        if (stricmp_utf8(aname.c_str(), selectedItem) == 0) {
          rowId = uSendDlgItemMessageText(hWnd, id, CB_FINDSTRINGEXACT, 1, selectedItem);
          rowId = uSendDlgItemMessageText(hWnd, id, CB_SETCURSEL, rowId, 0);
        }
      }
    }
    //fix the combo if still undefined
    int checkId = uSendDlgItemMessage(hWnd, id, CB_GETCURSEL, 0, 0);
    if (checkId == CB_ERR) {
      int rowId = uSendDlgItemMessageText(hWnd, id, CB_ADDSTRING, 0, selectedItem);
      uSendDlgItemMessageText(hWnd, id, CB_SETCURSEL, rowId, 0);
    }

    SetWindowRedraw(list, true);
  }
};
extern ListMap customCoverFrontArtMap;
/**************************************************************************************/
/**************************************************************************************/
class DisplayTab : public ConfigTab {
 public:
  CONFIG_TAB(DisplayTab, "Display", IDD_DISPLAY_TAB);

  bool browseForImage(const char* oldImg, pfc::string_base& out) {
    OPENFILENAME ofn;
    wchar_t fileName[1024];
    pfc::stringcvt::convert_utf8_to_wide(
        static_cast<wchar_t*>(fileName), 1024, oldImg, ~0u);
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = static_cast<wchar_t*>(fileName);
    ofn.nMaxFile = 1024;
    ofn.lpstrFilter = L"Image File\0*.jpg;*.jpeg;*.png;*.gif;*.bmp;*.tiff\0";
    ofn.nFilterIndex = 1;
    ofn.Flags =
        OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST;
    if (GetOpenFileName(&ofn)) {
      out = pfc::stringcvt::string_utf8_from_wide(static_cast<wchar_t*>(fileName))
                .get_ptr();
      return true;
    } else {
      return false;
    }
  }

  BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) final {
    switch (uMsg) {
      case 132:  //?
        break;
      case WM_SETFOCUS:
        break;
      case WM_INITDIALOG: {
        notifyParent(id, CF_USER_CONFIG_NEWTAB);
        int selectedndx = InitCustomCoverArtComboBox(configData->CustomCoverFrontArt);
        loadCustomCoverFrontArtList(customCoverFrontArtMap.at(selectedndx).c_str());

        int titlePosH = static_cast<int>(floor(configData->TitlePosH * 100 + 0.5));
        int titlePosV = static_cast<int>(floor(configData->TitlePosV * 100 + 0.5));

        uSendDlgItemMessage(hWnd, IDC_TPOS_H, TBM_SETRANGE, FALSE, MAKELONG(0, 100));
        uSendDlgItemMessage(hWnd, IDC_TPOS_H, TBM_SETTIC, 0, 50);
        uSendDlgItemMessage(hWnd, IDC_TPOS_H, TBM_SETPOS, TRUE, titlePosH);
        uSendDlgItemMessageText(
            hWnd, IDC_TPOS_H_P, WM_SETTEXT, 0, std::to_string(titlePosH).data());

        uSendDlgItemMessage(hWnd, IDC_TPOS_V, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
        uSendDlgItemMessage(hWnd, IDC_TPOS_V, TBM_SETTIC, 0, 50);
        uSendDlgItemMessage(hWnd, IDC_TPOS_V, TBM_SETPOS, TRUE, titlePosV);
        uSendDlgItemMessageText(
            hWnd, IDC_TPOS_V_P, WM_SETTEXT, 0, std::to_string(titlePosV).data());

        uSendDlgItemMessage(
            hWnd, IDC_FRAME_WIDTH_SPIN, UDM_SETRANGE, 0, MAKELONG(short(30), short(0)));
        uSetDlgItemInt(
          hWnd, IDC_FRAME_WIDTH, configData->HighlightWidth, false);
        
        break;
      }
      case WM_HSCROLL: {
        CScrollBar* sb = reinterpret_cast<CScrollBar*>(&lParam);
        UINT idscrollcontrol = sb->GetDlgCtrlID();

        UINT idtextcontrol, idtextdoublecontrol;

        if (idscrollcontrol == IDC_TPOS_H) {
          idtextcontrol = IDC_TPOS_H_P;
          idtextdoublecontrol = IDC_HIDDEN_TPOS_H;
        } else {
          idtextcontrol = IDC_TPOS_V_P;
          idtextdoublecontrol = IDC_HIDDEN_TPOS_V;
        }
        const int val = uSendDlgItemMessage(hWnd, idscrollcontrol, TBM_GETPOS, 0, 0);
        uSetDlgItemInt(hWnd, idtextcontrol, val, true);

        //will trigger EN_CHANGE & therefore textchanged(id)
        uSetDlgItemText(hWnd, idtextdoublecontrol, std::to_string(val * 0.01).c_str());
        
      } break;
      case WM_DRAWITEM: {
        auto* drawStruct = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        HBRUSH brush{};
        switch (wParam) {
          unsigned long along;
          case IDC_TEXTCOLOR_PREV: {
            COLORREF textColor = std::stoull(
                uGetDlgItemText(hWnd, IDC_HIDDEN_TITLE_COLOR_CUSTOM).c_str(), nullptr, 0);
            brush = CreateSolidBrush(textColor);
            break;
          }
          case IDC_BG_COLOR_PREV: {
            COLORREF panelBg = std::stoull(
                uGetDlgItemText(hWnd, IDC_HIDDEN_PANELBG_CUSTOM).c_str(), nullptr, 0);
            brush = CreateSolidBrush(panelBg);
            break;
          }
        }
        FillRect(drawStruct->hDC, &drawStruct->rcItem, brush);
      } break;
      case WM_COMMAND:
        if (HIWORD(wParam) == EN_ERRSPACE) {
          FB2K_console_formatter()
              << "EN_ERRSPACE " + LOWORD(wParam) << ", " << HIWORD(wParam);
        } else if (HIWORD(wParam) == EN_CHANGE) {
          switch (LOWORD(wParam)) {
            case IDC_IMG_NO_COVER:
            case IDC_IMG_LOADING:
            case IDC_HIDDEN_TPOS_V:
            case IDC_HIDDEN_TPOS_H:
            case IDC_HIDDEN_DISPLAY_CUSTOM_COVER_ART:
              textChanged(LOWORD(wParam));
              break;

            case IDC_ALBUM_FORMAT: {
              metadb_handle_ptr aTrack;
              if (metadb::get()->g_get_random_handle(aTrack)) {
                pfc::string8 preview;
                titleformat_object::ptr titleformat;
                pfc::string stralbumtitle = uGetDlgItemText(hWnd, IDC_ALBUM_FORMAT);
                titleformat_compiler::get()->compile_safe_ex(
                    titleformat, stralbumtitle.c_str());
                aTrack->format_title(nullptr, preview, titleformat, nullptr);
                preview.replace_string("\r\n", "↵");
                uSendDlgItemMessageText(hWnd, IDC_TITLE_PREVIEW, WM_SETTEXT, 0, preview);
              }
              textChanged(LOWORD(wParam));
            } break;
            case IDC_FRAME_WIDTH: {
              int val = uGetDlgItemInt(hWnd, IDC_FRAME_WIDTH, nullptr, false);
              int val_clamp = std::clamp(val, 0, 30);
              if (val != val_clamp) {
                uSetDlgItemInt(hWnd, IDC_FRAME_WIDTH, val_clamp, true);
              }
              textChanged(IDC_FRAME_WIDTH);
            } break;
          }
        } else if (HIWORD(wParam) == BN_CLICKED) {
          switch (LOWORD(wParam)) {
            case IDC_APPLY_TITLE: {
              EngineThread::forEach(
                  [](EngineThread& t) { t.send<EM::ReloadCollection>(); });
            } break;
            case IDC_BG_COLOR: {
              COLORREF panelBg = std::stoull(
                  uGetDlgItemText(hWnd, IDC_HIDDEN_PANELBG_CUSTOM).c_str(), nullptr, 0);
              ;
              if (selectColor(panelBg)) {
                uSetDlgItemText(
                    hWnd, IDC_HIDDEN_PANELBG_CUSTOM, std::to_string(panelBg).data());
                InvalidateRect(uGetDlgItem(hWnd, IDC_BG_COLOR_PREV), nullptr, TRUE);
                textChanged(IDC_HIDDEN_PANELBG_CUSTOM);
              }
            } break;
            case IDC_TEXTCOLOR: {
              COLORREF titleColor = std::stoull(
                  uGetDlgItemText(hWnd, IDC_HIDDEN_TITLE_COLOR_CUSTOM).c_str(), nullptr,
                  0);

              if (selectColor(titleColor)) {
                uSetDlgItemText(hWnd, IDC_HIDDEN_TITLE_COLOR_CUSTOM,
                                std::to_string(titleColor).data());
                InvalidateRect(uGetDlgItem(hWnd, IDC_TEXTCOLOR_PREV), nullptr, TRUE);
                EngineThread::forEach(
                    [](EngineThread& t) { t.send<EM::TextFormatChangedMessage>(); });
                textChanged(IDC_HIDDEN_TITLE_COLOR_CUSTOM);
              }
            } break;
            case IDC_FONT: {
              LOGFONT titleFont = configData->TitleFont;
              if (selectFont(titleFont)) {
                std::ostringstream o_txt_stream;
                o_txt_stream.write((char*)&titleFont, sizeof(titleFont));
                pfc::string8 strBase64;
                pfc::base64_encode(
                    strBase64, (void*)o_txt_stream.str().c_str(), sizeof(titleFont));

                uSetDlgItemText(hWnd, IDC_HIDDEN_LOGFONT, strBase64.c_str());

                // debug
                // pfc::string8 tmpstrBase64;
                // uGetDlgItemText(hWnd, IDC_HIDDEN_LOGFONT, tmpstrBase64);
                // LOGFONT restoreFont;
                // pfc::base64_decode(tmpstrBase64.c_str(), &restoreFont);

                uSendDlgItemMessage(hWnd, IDC_FONT_PREV, WM_SETTEXT, 0,
                                    reinterpret_cast<LPARAM>(titleFont.lfFaceName));
                EngineThread::forEach(
                    [](EngineThread& t) { t.send<EM::TextFormatChangedMessage>(); });

                textChanged(LOWORD(wParam));
              }
            } break;
            case IDC_IMG_NO_COVER_BROWSE: {
              pfc::string8 strDlgOutString;
              if (browseForImage(configData->ImgNoCover, strDlgOutString)) {
                uSetDlgItemText(hWnd, IDC_IMG_NO_COVER, strDlgOutString);
              }
            } break;
            case IDC_IMG_LOADING_BROWSE: {
              pfc::string8 strDlgOutString;
              if (browseForImage(configData->ImgLoading, strDlgOutString)) {
                uSetDlgItemText(hWnd, IDC_IMG_LOADING, strDlgOutString);
              }
            } break;
            case IDC_ALBUM_TITLE:
            case IDC_TEXTCOLOR_CUSTOM:
            case IDC_FONT_CUSTOM:
            case IDC_BG_COLOR_CUSTOM:
            case IDC_COVER_ART_PNG8_ALPHA: 
            case IDC_COVER_DISPLAY_LEGACY_EXTVIEWER: {
              buttonClicked(LOWORD(wParam));

              EngineThread::forEach(
                  [](EngineThread& t) { t.send<EM::TextFormatChangedMessage>(); });
            } break;
          }

        } else if (HIWORD(wParam) == CBN_SELCHANGE) {
          if (LOWORD(wParam) == IDC_PANEL_DISPLAY_COMBO_CUSTOM_COVER_ART) {
            int itemId = uSendDlgItemMessage(hWnd, LOWORD(wParam), CB_GETCURSEL, 0, 0);
            if (itemId == CB_ERR) {
              return FALSE;
            }
            int val = uSendDlgItemMessage(hWnd, LOWORD(wParam), CB_GETITEMDATA, itemId, 0);
            // will trigger textchanged...
            uSetDlgItemText(
                hWnd, IDC_HIDDEN_DISPLAY_CUSTOM_COVER_ART, std::to_string(val).data());

          }
        } else {
          //int a = LOWORD(wParam);
          //int b = HIWORD(wParam);
        }
        break;
    }
    return FALSE;
  }

 private:

  bool selectColor(COLORREF& color) {
    static DWORD costumColors[16] = {0};
    COLORREF tColor = color;
    if (uChooseColor(&tColor, hWnd, static_cast<DWORD*>(costumColors))) {
      color = tColor;
      return true;
    } else {
      return false;
    }
  }

  bool selectFont(LOGFONT& font) {
    LOGFONT tFont = font;
    CHOOSEFONT cf = {0};

    cf.lStructSize = sizeof(cf);
    cf.hwndOwner = hWnd;
    cf.lpLogFont = &tFont;
    cf.Flags = CF_TTONLY | CF_SCREENFONTS | CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT;
    if (ChooseFont(&cf)) {
      font = tFont;
      return true;
    } else {
      return false;
    }
  }

  void loadCustomCoverFrontArtList(const char* selectedItem) {
    UINT id = IDC_PANEL_DISPLAY_COMBO_CUSTOM_COVER_ART;
    HWND list = GetDlgItem(hWnd, id);

    SetWindowRedraw(list, false);
    SendMessage(list, CB_RESETCONTENT, 0, 0);
    uSendMessageText(list, CB_ADDSTRING, 0, "");
    uSendDlgItemMessage(hWnd, id, CB_RESETCONTENT, 0, 0);

    // cover_front, cover_back, disc, artist
    for (auto& [val, text] : customCoverFrontArtMap) {
      int itemId = uSendDlgItemMessageText(hWnd, id, CB_ADDSTRING, 0, text.c_str());
      uSendDlgItemMessage(hWnd, id, CB_SETITEMDATA, itemId, val);
      if (stricmp_utf8(text.c_str(), selectedItem) == 0) {
        uSendDlgItemMessageText(hWnd, id, CB_SELECTSTRING, 1, selectedItem);
      }
    }
    SetWindowRedraw(list, true);
  }

  int InitCustomCoverArtComboBox(int cfgval) {
      UINT idcontrol = IDC_PANEL_DISPLAY_COMBO_CUSTOM_COVER_ART;
      int selectedndx = cfgval >= customCoverFrontArtMap.size() ? 0 : cfgval;
      pfc::string8 textSelection = customCoverFrontArtMap.at(selectedndx).c_str();

      uSendDlgItemMessageText(hWnd, idcontrol, CB_SELECTSTRING, 1, textSelection.c_str());
      HWND hwndCombo = uGetDlgItem(hWnd, idcontrol);
      return selectedndx;
  }

};

class ConfigNameDialog : private dialog_helper::dialog_modal {
  BOOL on_message(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/) final {
    switch (uMsg) {
      case WM_INITDIALOG:
        if (value != nullptr) {
          uSetDlgItemText(get_wnd(), IDC_CONFIG_NAME, value);
        }
        SetFocus(uGetDlgItem(get_wnd(), IDC_CONFIG_NAME));
        return 0;
      case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED) {
          if (LOWORD(wParam) == IDOK) {
            end_dialog(1);
          } else if (LOWORD(wParam) == IDCANCEL) {
            end_dialog(0);
          }
        } else if (HIWORD(wParam) == EN_CHANGE) {
          uGetDlgItemText(get_wnd(), IDC_CONFIG_NAME, value);
        }
        return 0;
    }
    return 0;
  }

 public:
  int query(HWND parent, const char* defValue = "") {
    value = defValue;
    return run(IDD_CONFIG_NAME, parent);  // NOLINT
  }

  pfc::string8 value;
};

/**************************************************************************************/
/**************************************************************************************/
class CoverTab : public ConfigTab {
  HFONT editBoxFont{};
  WNDPROC origEditboxProc = nullptr;
  const char** p = builtInCoverConfigArray;

  pfc::string8 m_edited_name;
  int m_edited_ndx;

 public:
  CONFIG_TAB(CoverTab, "Cover Display", IDD_COVER_DISPLAY_TAB);

  BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/) final {
    switch (uMsg) {
      case WM_INITDIALOG: {
        notifyParent(id, CF_USER_CONFIG_NEWTAB);
        m_edited_name = "";
        m_edited_ndx = -1;
        editBoxFont =
            CreateFont(-12, 0, 0, 0, 0, FALSE, 0, 0, 0, 0, 0, 0, 0, L"Courier New");

        loadComboConfigList(-1);

        const CoverConfig& config =
            configData->CoverConfigs.at(configData->CoverConfigSel.c_str());
        uSendDlgItemMessage(hWnd, IDC_DISPLAY_CONFIG, EM_SETREADONLY,
                            static_cast<int>(config.buildIn), 0);
        return TRUE;
      }
      case WM_DESTROY:
        DeleteObject(editBoxFont);
      case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE) {
          if (LOWORD(wParam) == IDC_COMPILE_STATUS) {
            return FALSE;
          }
          if (LOWORD(wParam) == IDC_DISPLAY_CONFIG) {
            uSetDlgItemText(hWnd, IDC_COMPILE_STATUS, "");
          }
          textChanged(LOWORD(wParam));
        } else
        if (HIWORD(wParam) == BN_CLICKED) {
          switch (LOWORD(wParam)) {
            case IDC_SAVED_ADD:
              addConfig();
              textChanged(IDC_SAVED_SELECT);
              break;
            case IDC_SAVED_RENAME:
              renameConfig();
              textChanged(IDC_SAVED_SELECT);
              break;
            case IDC_SAVED_REMOVE:
              removeConfig();
              textChanged(IDC_SAVED_SELECT);
              break;
            case IDC_COMPILE:
              compileConfig();
              break;
            default:
              // other clicks
              break;
          }
        } else
        if (HIWORD(wParam) == CBN_SELCHANGE) {
          if (LOWORD(wParam) == IDC_SAVED_SELECT) {
            loadEditBoxScript();             //update script edit box & delete/rename buttons
            listSelChanged(LOWORD(wParam));  //update apply button
          }
        }
        break;
    }
    return FALSE;
  }

  void compileConfig() {
    pfc::string8 script;
    uGetDlgItemText(hWnd, IDC_DISPLAY_CONFIG, script);

    try {
      compileCPScript(script);
      uSetDlgItemText(hWnd, IDC_COMPILE_STATUS, "Compilation successful");
    } catch (std::exception& e) {
      uSetDlgItemText(hWnd, IDC_COMPILE_STATUS, e.what());
    }
  }

  void setUpEditBox() {
    int tabstops[1] = {14};
    SendDlgItemMessage(
        hWnd, IDC_DISPLAY_CONFIG, EM_SETTABSTOPS, 1, reinterpret_cast<LPARAM>(tabstops));
    SendDlgItemMessage(hWnd, IDC_DISPLAY_CONFIG, WM_SETFONT,
                       reinterpret_cast<WPARAM>(editBoxFont), TRUE);
    origEditboxProc = reinterpret_cast<WNDPROC>(
        SetWindowLong(GetDlgItem(hWnd, IDC_DISPLAY_CONFIG), GWL_WNDPROC,
                      reinterpret_cast<LONG>(editboxProxy)));
    SetProp(GetDlgItem(hWnd, IDC_DISPLAY_CONFIG), L"tab", static_cast<HANDLE>(this));
  }

  static BOOL CALLBACK editboxProxy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto* coverTab = reinterpret_cast<CoverTab*>(GetProp(hWnd, L"tab"));
    return coverTab->editboxProc(hWnd, uMsg, wParam, lParam);
  }

  BOOL editboxProc(HWND eWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYDOWN && wParam == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
      SendMessage(eWnd, EM_SETSEL, 0, -1);
      return 0;
    }

    if (GetWindowLong(eWnd, GWL_STYLE) & ES_READONLY)
      return CallWindowProc(origEditboxProc, eWnd, uMsg, wParam, lParam);

    if (uMsg == WM_KEYDOWN) {
      if (wParam == VK_TAB) {
        DWORD selFirst;
        DWORD selLast;
        SendMessage(eWnd, EM_GETSEL, reinterpret_cast<WPARAM>(&selFirst),
                    reinterpret_cast<LPARAM>(&selLast));
        if (selFirst == selLast) {
          uSendMessageText(eWnd, EM_REPLACESEL, TRUE, "\t");
        } else {
          bool intend = (GetKeyState(VK_SHIFT) & 0x8000) == 0;
          pfc::string8 boxText;
          uGetWindowText(eWnd, boxText);
          PFC_ASSERT(selLast <= boxText.length());
          const char* pStart = boxText.get_ptr();
          const char* selEnd = pStart + selLast;
          const char* p = pStart + selFirst;
          while ((p > pStart) && (p[-1] != '\n')) p--;
          unsigned int repFirst = p - pStart;
          if (intend) {
            selFirst++;
          } else {
            if (*p == '\t') {
              selFirst--;
            } else if ((p + 2 < selEnd) && p[0] == ' ' && p[1] == ' ' && p[2] == ' ') {
              selFirst -= 3;
            }
            if (selFirst < repFirst)
              selFirst = repFirst;
          }
          pfc::string8 outText;
          outText.prealloc(selEnd - p);
          const char* lStart = p;
          p++;
          while (p <= selEnd) {
            while (p[-1] != '\n' && p < selEnd) p++;
            if (intend) {
              outText.add_char('\t');
            } else {
              if (*lStart == '\t') {
                lStart++;
              } else if ((lStart + 3 < p) && lStart[0] == ' ' && lStart[1] == ' ' &&
                         lStart[2] == ' ') {
                lStart += 3;
              }
            }
            outText.add_string(lStart, p - lStart);
            lStart = p;
            p++;
          }
          SendMessage(eWnd, EM_SETSEL, repFirst, selLast);
          uSendMessageText(eWnd, EM_REPLACESEL, TRUE, outText);
          SendMessage(eWnd, EM_SETSEL, selFirst, repFirst + outText.length());
        }
        return 0;
      } else if (wParam == VK_RETURN) {
        DWORD selFirst;
        DWORD selLast;
        SendMessage(eWnd, EM_GETSEL, reinterpret_cast<WPARAM>(&selFirst),
                    reinterpret_cast<LPARAM>(&selLast));
        pfc::string8 boxText;
        uGetWindowText(eWnd, boxText);
        const char* pStart = boxText.get_ptr();
        const char* selEnd = pStart + selLast;
        const char* p = pStart + selFirst;
        int tabCount = 0;
        if (p > pStart && p[-1] == '{')
          tabCount++;
        while ((p > pStart) && (p[-1] != '\n')) p--;
        while (*p == '\t' ||
               ((p + 2 < selEnd) && *p == ' ' && *(++p) == ' ' && *(++p) == ' ')) {
          p++;
          tabCount++;
        }
        pfc::string8 intend("\r\n");
        intend.add_chars('\t', tabCount);
        uSendMessageText(eWnd, EM_REPLACESEL, TRUE, intend);
        return 0;
      }
    } else if (uMsg == WM_CHAR) {
      if (wParam == 1) {
        return 0;
      } else if (wParam == VK_RETURN) {
        return 0;
      } else if (wParam == '}') {
        wchar_t line[3];
        line[0] = 3;
        DWORD selFirst;
        SendMessage(eWnd, EM_GETSEL, reinterpret_cast<WPARAM>(&selFirst), 0);
        int lineNumber = SendMessage(eWnd, EM_LINEFROMCHAR, selFirst, 0);
        int lineLength = SendMessage(eWnd, EM_LINELENGTH, selFirst, 0);
        int lineIdx = SendMessage(eWnd, EM_LINEINDEX, lineNumber, 0);
        SendMessage(eWnd, EM_GETLINE, lineNumber, reinterpret_cast<LPARAM>(&line));
        uSendMessageText(eWnd, EM_REPLACESEL, TRUE, "}");
        int deleteChars = 0;
        if (lineLength > 0 && line[0] == '\t') {
          deleteChars = 1;
        } else if (lineLength > 2 && line[0] == ' ' && line[1] == ' ' && line[2] == ' ') {
          deleteChars = 3;
        }
        if (deleteChars > 0) {
          SendMessage(eWnd, EM_SETSEL, lineIdx, (lineIdx + deleteChars));
          uSendMessageText(eWnd, EM_REPLACESEL, TRUE, "");
          SendMessage(
              eWnd, EM_SETSEL, selFirst - deleteChars + 1, selFirst - deleteChars + 1);
        }
        return 0;
      }
    }
    return CallWindowProc(origEditboxProc, eWnd, uMsg, wParam, lParam);
  }

  void removeConfig() {
    if (configData->CoverConfigs.size() > 1) {
      pfc::string8 oldselection;
      pfc::string8 title;
      uGetDlgItemText(hWnd, IDC_SAVED_SELECT, oldselection);
      title << "Delete Config \"" << oldselection << "\"";
      if (IDYES == uMessageBox(hWnd, "Are you sure?", title,
                               MB_APPLMODAL | MB_YESNO | MB_ICONQUESTION)) {
        auto i = configData->CoverConfigs.find(oldselection.c_str());
        auto next = configData->CoverConfigs.erase(i);
        if (next == configData->CoverConfigs.end())
          --next;
        int newselection_ndx =
            GetCoverConfigPosition(configData->CoverConfigs, next->first.c_str());
        HWND ctrl = GetDlgItem(hWnd, IDC_SAVED_SELECT);
        int oldselection_ndx = uSendMessage(ctrl, CB_GETCURSEL, 0, 0);
        
        m_edited_name = oldselection;
        m_edited_ndx = oldselection_ndx;
        //refresh combobox list
        loadComboConfigList(newselection_ndx);
        //refresh script edit box
        loadEditBoxScript();
      }
    }
  }

  void addConfig() {
    ConfigNameDialog dialog;
    if (dialog.query(hWnd)) {
      dialog.value.skip_trailing_char(' ');
      if (dialog.value.get_length()) {
        if (configData->CoverConfigs.count(dialog.value.c_str()) == 0) {
          auto script = builtInCoverConfigs().at(coverConfigTemplate).script;
          auto name = dialog.value;
          configData->CoverConfigs.insert({name.c_str(), CoverConfig{script, false}});

          int newselection_ndx =
              GetCoverConfigPosition(configData->CoverConfigs, name.c_str());
          // refresh combobox list
          loadComboConfigList(newselection_ndx);
          // refresh script edit box
          loadEditBoxScript();
        }
      }
    }
  }

  void renameConfig() {
    ConfigNameDialog dialog;
    pfc::string8 oldselection;
    uGetDlgItemText(hWnd, IDC_SAVED_SELECT, oldselection);
    int oldselected_ndx = uSendDlgItemMessage(hWnd, IDC_SAVED_SELECT, CB_GETCURSEL, 0, 0);
    if (dialog.query(hWnd, oldselection)) {
      dialog.value.skip_trailing_char(' ');
      if (dialog.value.get_length()) {
        auto existing = configData->CoverConfigs.find(dialog.value.c_str());
        if (existing == configData->CoverConfigs.end() ||
            existing->first == oldselection.c_str()) {
          auto node = configData->CoverConfigs.extract(oldselection.c_str());
          if (node.empty())
            return;
          node.key() = dialog.value.c_str();
          configData->CoverConfigs.insert(std::move(node));

          int newselection_ndx =
              GetCoverConfigPosition(configData->CoverConfigs, dialog.value.c_str());

          m_edited_name = oldselection;
          m_edited_ndx = oldselected_ndx;
          // refresh combobox list
          loadComboConfigList(newselection_ndx);
        }
      }
    }
  }

  void loadComboConfigList(int selection) {
    int selection_ndx = -1;
    pfc::string8 selection_name = "";
    int default_ndx = GetCoverConfigPosition(configData->CoverConfigs,
                                             configData->CoverConfigSel);

    HWND ctrl = GetDlgItem(hWnd, IDC_SAVED_SELECT);
    SetWindowRedraw(ctrl, false);
    SendMessage(ctrl, CB_RESETCONTENT, 0, 0);
    for (auto& [name, config] : configData->CoverConfigs) {
      int itemId =uSendMessageText(ctrl, CB_ADDSTRING, 0, name.c_str());
      uSendMessage(ctrl, CB_SETITEMDATA, itemId, itemId);

      if (selection!= -1 && itemId == selection) {
        selection_ndx = itemId;
        selection_name = name.c_str();
        uSendDlgItemMessageText(hWnd, IDC_SAVED_SELECT, CB_SELECTSTRING, 1, name.c_str());
      } 
    }

    if (selection == -1 || selection_ndx == -1) {
      selection_ndx = default_ndx;
      selection_name = configData->CoverConfigSel;
      uSendDlgItemMessageText(hWnd, IDC_SAVED_SELECT, CB_SELECTSTRING, 1, selection_name);
      //int default_ndx = uSendMessageText(ctrl, CB_FINDSTRINGEXACT, 1, configData->CoverConfigSel);
    }

    bool comboedited = (m_edited_name != "");

    if (comboedited) {
      int session_ndx = configData->sessionCompiledCPInfo.get().first;
      if (m_edited_ndx == session_ndx) {
        configData->sessionCompiledCPInfo.set(
            selection_ndx, configData->sessionCompiledCPInfo.get().second);
      }

      m_edited_name = "";
      m_edited_ndx = -1;
    }
    const CoverConfig& config = configData->CoverConfigs.at(selection_name.c_str());
    uEnableWindow(uGetDlgItem(hWnd, IDC_SAVED_REMOVE),
                  static_cast<BOOL>(!config.buildIn && selection_ndx!=default_ndx));
    uEnableWindow(uGetDlgItem(hWnd, IDC_SAVED_RENAME),
                  static_cast<BOOL>(!config.buildIn && selection_ndx!=default_ndx ));
    SetWindowRedraw(ctrl, true);
  }

  void loadEditBoxScript() {
    HWND ctrl = GetDlgItem(hWnd, IDC_SAVED_SELECT);
    try {

      pfc::string8 selected;
      uGetDlgItemText(hWnd, IDC_SAVED_SELECT, selected);

      //todo: not as expected...
      //int select_ndx = uSendDlgItemMessage(hWnd, IDC_SAVED_SELECT, CB_GETCURSEL, 0, 0);
      //uSendMessageText(ctrl, CB_GETLBTEXT, select_ndx, selected.get_ptr());

      const CoverConfig& config = configData->CoverConfigs.at(selected.c_str());
      pfc::string8 default = configData->CoverConfigSel;
      bool bdefault = (stricmp_utf8(selected, default) == 0);

      //int default_ndx = configData->GetCCPosition();
      //bdefault = select_ndx == default_ndx;

      uSetDlgItemText(
          hWnd, IDC_DISPLAY_CONFIG, windows_lineendings(config.script).c_str());
      uSendDlgItemMessage(
          hWnd, IDC_DISPLAY_CONFIG, EM_SETREADONLY, static_cast<int>(config.buildIn), 0);
      uEnableWindow(uGetDlgItem(hWnd, IDC_SAVED_REMOVE),
                    static_cast<BOOL>(!config.buildIn && !bdefault));
      uEnableWindow(uGetDlgItem(hWnd, IDC_SAVED_RENAME),
                    static_cast<BOOL>(!config.buildIn && !bdefault));
    } catch (std::out_of_range&) {
    }
    uSetDlgItemText(hWnd, IDC_COMPILE_STATUS, "");
  }
};

using ListMap = std::unordered_map<int, std::string>;
extern ListMap multisamplingMap;

static struct {
  int id;
  int* var;
  ListMap& map;
  // NOLINTNEXTLINE(cppcoreguidelines-interfaces-global-init)

} mappedListVarMap[] = {
    {IDC_MULTI_SAMPLING_PASSES, (int*)&configData->MultisamplingPasses, multisamplingMap},
};

/**************************************************************************************/
/**************************************************************************************/
class PerformanceTab : public ConfigTab {
 public:
  CONFIG_TAB(PerformanceTab, "Performance", IDD_PERF_TAB);

  BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/) final {
    switch (uMsg) {
      case WM_INITDIALOG: {
        notifyParent(id, CF_USER_CONFIG_NEWTAB);
        fillComboBoxes();

        SendDlgItemMessage(hWnd, IDC_CACHE_SIZE_SPIN, UDM_SETRANGE32, 2, 999);
        SendDlgItemMessage(hWnd, IDC_TEXTURE_SIZE_SPIN, UDM_SETRANGE32, 4, 2024);

        switch (configData->VSyncMode) {
          case VSYNC_SLEEP_ONLY:
            uButton_SetCheck(hWnd, IDC_VSYNC_OFF, true);
            break;
          case VSYNC_ONLY:
            uButton_SetCheck(hWnd, IDC_VSYNC_ONLY, true);
            break;
          case VSYNC_AND_SLEEP:
            uButton_SetCheck(hWnd, IDC_VSYNC_SLEEP, true);
            break;
          default:
            break;
        }
      } break;
      case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE) {
          if (LOWORD(wParam) == IDC_CACHE_SIZE) {
            int cache_size = uGetDlgItemInt(hWnd, IDC_CACHE_SIZE, nullptr, true);
            int cache_size_clamp = std::clamp(cache_size, 2, 999);
            if (cache_size != cache_size_clamp) {
              uSetDlgItemInt(hWnd, IDC_CACHE_SIZE, cache_size_clamp, true);
            }
            textChanged(IDC_CACHE_SIZE);
          } else if (LOWORD(wParam) == IDC_TEXTURE_SIZE) {
            int texture_size = uGetDlgItemInt(hWnd, IDC_TEXTURE_SIZE, nullptr, true);
            int texture_size_clamp = std::clamp(texture_size, 4, 2024);
            if (texture_size != texture_size_clamp) {
              uSetDlgItemInt(hWnd, IDC_TEXTURE_SIZE, texture_size_clamp, true);
            }
            textChanged(IDC_TEXTURE_SIZE);
          }
        } else if (HIWORD(wParam) == BN_CLICKED) {
          buttonClicked(LOWORD(wParam));

          switch (LOWORD(wParam)) {
            case IDC_VSYNC_OFF:
            case IDC_VSYNC_ONLY:
            case IDC_VSYNC_SLEEP:
              if (uButton_GetCheck(hWnd, IDC_VSYNC_OFF)) {
                uSetDlgItemInt(hWnd, IDC_HIDDEN_VSYNC_MODE, VSYNC_SLEEP_ONLY, true);
              } else if (uButton_GetCheck(hWnd, IDC_VSYNC_ONLY)) {
                uSetDlgItemInt(hWnd, IDC_HIDDEN_VSYNC_MODE, VSYNC_ONLY, true);
              } else if (uButton_GetCheck(hWnd, IDC_VSYNC_SLEEP)) {
                uSetDlgItemInt(hWnd, IDC_HIDDEN_VSYNC_MODE, VSYNC_AND_SLEEP, true);
              }
              textChanged(IDC_HIDDEN_VSYNC_MODE);
          }
          redrawMainWin();
        } else if (HIWORD(wParam) == CBN_SELCHANGE) {
          comboBoxChanged(LOWORD(wParam));
          redrawMainWin();
        }
        break;
      default:
        break;
    }
    return FALSE;
  }

  void fillComboBoxes() {
    for (auto& i : mappedListVarMap) {
      uSendDlgItemMessage(hWnd, i.id, CB_RESETCONTENT, 0, 0);
      for (auto& [val, text] : i.map) {
        uSendDlgItemMessageText(hWnd, i.id, CB_ADDSTRING, 0, text.c_str());
        if (val == *(i.var)) {
          uSendDlgItemMessageText(hWnd, i.id, CB_SELECTSTRING, 1, text.c_str());
        }
      }
    }
  }

  void comboBoxChanged(int comboBox) {
    pfc::string8 selected;
    int s = uSendDlgItemMessage(hWnd, comboBox, CB_GETCURSEL, 0, 0);
    if (s != CB_ERR) {
      //(uGetDlgItem(hWnd, comboBox), s, selected);
    } else {
      return;
    }

    notifyParent(comboBox, CF_USER_CONFIG_CHANGE);
  }
};

/**************************************************************************************/
/**************************************************************************************/
class ContextMenuTab : public ConfigTab {
 public:
  CONFIG_TAB(ContextMenuTab, "Context Menu", IDD_CONTEXT_MENU_TAB);
  HFONT editBoxFont{};
  const char* p = docuconfig;
  BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/) final {
    switch (uMsg) {
      case WM_INITDIALOG: {
        notifyParent(id, CF_USER_CONFIG_NEWTAB);

      } break;
      case WM_DESTROY:
        break;
      case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED) {
          buttonClicked(LOWORD(wParam));

          switch (LOWORD(wParam)) {
          }

        }
        break;
    }
    return FALSE;
  }
};

/**************************************************************************************/
/**************************************************************************************/
class ControlsTab : public ConfigTab {
 public:
  CONFIG_TAB(ControlsTab, "Help File", IDD_CONTROLS_TAB);
  HFONT editBoxFont{};
  const char* p = docuconfig;
  BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/) final {
    switch (uMsg) {
      case WM_INITDIALOG: {
        notifyParent(id, CF_USER_CONFIG_NEWTAB);
        uSetDlgItemText(hWnd, IDC_CONTROLS_EDIT_DOCU, windows_lineendings(p).c_str());
      }
      case WM_DESTROY:
        DeleteObject(editBoxFont);
      case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE) {
        } 
        break;
    }
    return FALSE;
  }
  void setUpEditBox() {
    int tabstops[1] = {14};
    SendDlgItemMessage(hWnd, IDC_CONTROLS_EDIT_DOCU, EM_SETTABSTOPS, 1,
                       reinterpret_cast<LPARAM>(tabstops));
    SendDlgItemMessage(hWnd, IDC_CONTROLS_EDIT_DOCU, WM_SETFONT,
                       reinterpret_cast<WPARAM>(editBoxFont), TRUE);
    SetProp(GetDlgItem(hWnd, IDC_CONTROLS_EDIT_DOCU), L"tab", static_cast<HANDLE>(this));
  }
};

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

class ConfigWindow {
 private:
  std::vector<unique_ptr<ConfigTab>> tabs;
  t_size currentTab;
  HWND m_wndParent;
  RECT m_rcTab;

 public:
  ConfigWindow() : currentTab(~0u), m_rcTab() {}

  ConfigWindow(HWND wndParent) : m_wndParent(wndParent), currentTab(~0u), m_rcTab() {}

  void OnDestroy() {
    // won't be automatically called by fb2k, invoked from ConfigDialog destroy event
    configData->sessionSelectedConfigTab = currentTab;
    // dont tabs.clear(), etc... fb2k should take care of that
  }

  ~ConfigWindow() {}

  t_size GetCurrentTab() { return currentTab; }

  HWND GetTabWindow(t_size ndx) {
    ConfigTab* ct = tabs[ndx].get();
    return ct->getTabWnd();
  }

  HWND GetCurrentTabWnd() {
    return GetTabWindow(GetCurrentTab());
  }
  
  ConfigTab* GetCurrentConfigTab() {
    ConfigTab* ct = tabs[GetCurrentTab()].get();
    return ct;
  }
  // ui
  void LoadDisableMap() {
      ConfigTab* configTab = tabs[currentTab].get();
      configTab->loadDisableMap();
  }

  // called by ConfigDialog::OnInitDialog()
  void OnInit(CWindow wndParent, LPARAM lInitParam) {
    this->m_wndParent = wndParent;
    HWND hWndTab = uGetDlgItem(m_wndParent, IDC_TABS);
    tabs.emplace_back(make_unique<BehaviourTab>(m_wndParent));
    tabs.emplace_back(make_unique<SourcesTab>(m_wndParent));
    tabs.emplace_back(make_unique<DisplayTab>(m_wndParent));
    tabs.emplace_back(make_unique<CoverTab>(m_wndParent));
    tabs.emplace_back(make_unique<PerformanceTab>(m_wndParent));
    tabs.emplace_back(make_unique<ContextMenuTab>(m_wndParent));
    tabs.emplace_back(make_unique<ControlsTab>(m_wndParent));
    currentTab = configData->sessionSelectedConfigTab;
    if (currentTab >= tabs.size())
      currentTab = 0;
    uSendMessage(hWndTab, TCM_SETCURSEL, currentTab, 0);
    tabs.at(currentTab)->show();
  }

  LRESULT ShowTab(int idCtrl, LPNMHDR pnmh) {
    const NMHDR& msg = *pnmh;
    if (msg.idFrom == IDC_TABS) {
      if (msg.code == TCN_SELCHANGE) {
        if (currentTab < tabs.size())
          tabs.at(currentTab)->hide();
        currentTab = SendDlgItemMessage(m_wndParent, IDC_TABS, TCM_GETCURSEL, 0, 0);
        if (currentTab < tabs.size()) {
          tabs.at(currentTab)->show();
        }
      }
    }
    return FALSE;
  }
};

/**************************************************************************************/
/**************************************************************************************/

class LibraryViewer : public library_viewer {
  GUID get_guid() final {
    // fb2k guid
    return {0xa68951b7, 0x1497, 0x4ed2, {0xa2, 0xba, 0xcf, 0xaf, 0xd, 0xaa, 0xb8, 0x84}};
  };
  const char* get_name() final { return component_NAME; };
  GUID get_preferences_page() final { return guid_config_window; };
  bool have_activate() final { return false; };

  void activate() final{};
};

static library_viewer_factory_t<LibraryViewer> g_libraryViewer;
}  // namespace coverflow
