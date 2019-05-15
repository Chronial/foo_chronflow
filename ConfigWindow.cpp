#include <boost/range/iterator_range.hpp>

#include <windowsx.h>

#include "../foobar2000/helpers/dialog_resize_helper.h"
#include "../foobar2000/helpers/win32_dialog.h"
#include "lib/win32_helpers.h"

#include "DbAlbumCollection.h"
#include "Engine.h"
#include "EngineThread.h"
#include "MyActions.h"
#include "PlaybackTracer.h"
#include "config.h"
#include "cover_positions_compiler.h"
#include "utils.h"

// NOLINTNEXTLINE(cppcoreguidelines-interfaces-global-init)
std::unordered_map<UINT, cfg_string*> stringVarMap{
    // Sources
    {IDC_FILTER, &cfgFilter},
    {IDC_GROUP, &cfgGroup},
    {IDC_SORT, &cfgSort},
    {IDC_INNER_SORT, &cfgInnerSort},
    {IDC_IMG_NO_COVER, &cfgImgNoCover},
    {IDC_IMG_LOADING, &cfgImgLoading},

    // Display
    {IDC_ALBUM_FORMAT, &cfgAlbumTitle},

    // Behaviour
    {IDC_TARGET_PL, &cfgTargetPlaylist},
};

// NOLINTNEXTLINE(cppcoreguidelines-interfaces-global-init)
std::unordered_map<UINT, cfg_string*> textListVarMap{
    // Behaviour
    {IDC_DOUBLE_CLICK, &cfgDoubleClick},
    {IDC_MIDDLE_CLICK, &cfgMiddleClick},
    {IDC_ENTER_KEY, &cfgEnterKey},

    // Cover Display
    {IDC_SAVED_SELECT, &cfgCoverConfigSel},
};

// NOLINTNEXTLINE(cppcoreguidelines-interfaces-global-init)
std::unordered_map<UINT, cfg_bool*> boolVarMap{
    // Sources
    {IDC_SORT_GROUP, &cfgSortGroup},

    // Behaviour
    {IDC_FOLLOW_PLAYBACK, &cfgCoverFollowsPlayback},
    {IDC_FIND_AS_YOU_TYPE, &cfgFindAsYouType},

    // Display
    {IDC_ALBUM_TITLE, &cfgShowAlbumTitle},

    // Performance
    {IDC_MULTI_SAMPLING, &cfgMultisampling},
    {IDC_TEXTURE_COMPRESSION, &cfgTextureCompression},
    {IDC_EMPTY_ON_MINIMIZE, &cfgEmptyCacheOnMinimize},
    {IDC_SHOW_FPS, &cfgShowFps},
};

std::multimap<UINT, int> disableMap{
    // Sources
    {IDC_SORT_GROUP, -IDC_SORT},

    // Behaviour
    {IDC_FOLLOW_PLAYBACK, IDC_FOLLOW_DELAY},

    // Performance
    {IDC_MULTI_SAMPLING, IDC_MULTI_SAMPLING_PASSES},
};

class ConfigTab {
 protected:
  UINT id;
  HWND parent;
  char* title;
  bool initializing{};

 public:
  HWND hWnd = nullptr;
  ConfigTab(char* title, UINT id, HWND parent) : id(id), parent(parent), title(title) {
    HWND hWndTab = uGetDlgItem(parent, IDC_TABS);
    uTCITEM tabItem = {0};
    tabItem.mask = TCIF_TEXT;
    tabItem.pszText = title;
    int idx = TabCtrl_GetItemCount(hWndTab);
    uTabCtrl_InsertItem(hWndTab, idx, &tabItem);
  }
  NO_MOVE_NO_COPY(ConfigTab);
  virtual ~ConfigTab() {
    if (hWnd != nullptr)
      DestroyWindow(hWnd);
  }

  virtual BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam) = 0;
  static BOOL CALLBACK dialogProxy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ConfigTab* configTab = nullptr;
    if (uMsg == WM_INITDIALOG) {
      configTab = reinterpret_cast<ConfigTab*>(lParam);
      SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(configTab));
      configTab->hWnd = hWnd;
    } else {
      configTab = reinterpret_cast<ConfigTab*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }
    if (configTab == nullptr)
      return FALSE;
    return configTab->dialogProc(hWnd, uMsg, wParam, lParam);
  }
  void show() {
    if (hWnd == nullptr) {
      hWnd = uCreateDialog(id, parent, dialogProxy, reinterpret_cast<LPARAM>(this));
      HWND hWndTab = uGetDlgItem(parent, IDC_TABS);

      RECT rcTab;
      GetChildWindowRect(parent, IDC_TABS, &rcTab);
      uSendMessage(hWndTab, TCM_ADJUSTRECT, FALSE, reinterpret_cast<LPARAM>(&rcTab));
      SetWindowPos(hWnd, nullptr, rcTab.left, rcTab.top, rcTab.right - rcTab.left,
                   rcTab.bottom - rcTab.top, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    ShowWindow(hWnd, SW_SHOW);
  }
  void hide() { ShowWindow(hWnd, SW_HIDE); }
  void loadConfig() {
    initializing = true;
    for (auto [id, var] : stringVarMap) {
      uSetDlgItemText(hWnd, id, var->get_ptr());
    }
    for (auto [id, var] : boolVarMap) {
      uButton_SetCheck(hWnd, id, var->get_value());
    }
    for (auto [checkboxId, itemToDisable] : disableMap) {
      bool enabled = uButton_GetCheck(hWnd, checkboxId);
      if (itemToDisable < 0) {
        uEnableWindow(uGetDlgItem(hWnd, -itemToDisable), static_cast<BOOL>(!enabled));
      } else {
        uEnableWindow(uGetDlgItem(hWnd, itemToDisable), static_cast<BOOL>(enabled));
      }
    }
    initializing = false;
  }
  void textChanged(UINT id) {
    if (initializing)
      return;
    if (stringVarMap.count(id) > 0) {
      pfc::string8 buf;
      uGetDlgItemText(hWnd, id, buf);
      *(stringVarMap[id]) = buf;
    }
  }
  void buttonClicked(UINT id) {
    if (boolVarMap.count(id) > 0) {
      *(boolVarMap[id]) = uButton_GetCheck(hWnd, id);
    }
    for (auto [checkboxId, itemToDisable] :
         boost::make_iterator_range(disableMap.equal_range(id))) {
      bool enabled = uButton_GetCheck(hWnd, checkboxId);
      if (itemToDisable < 0) {
        uEnableWindow(uGetDlgItem(hWnd, -itemToDisable), static_cast<BOOL>(!enabled));
      } else {
        uEnableWindow(uGetDlgItem(hWnd, itemToDisable), static_cast<BOOL>(enabled));
      }
    }
  }
  void listSelChanged(UINT id) {
    if (textListVarMap.count(id) > 0) {
      int s = uSendDlgItemMessage(hWnd, id, CB_GETCURSEL, 0, 0);
      if (s != CB_ERR) {
        uComboBox_GetText(uGetDlgItem(hWnd, id), s, *(textListVarMap[id]));
      }
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

class SourcesTab : public ConfigTab {
 public:
  CONFIG_TAB(SourcesTab, "Album Source", IDD_SOURCE_TAB);

  BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/) final {
    switch (uMsg) {
      case WM_INITDIALOG:
        loadConfig();
        break;

      case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE) {
          textChanged(LOWORD(wParam));
        } else if (HIWORD(wParam) == BN_CLICKED) {
          buttonClicked(LOWORD(wParam));
          switch (LOWORD(wParam)) {
            case IDC_BTN_REFRESH:
              EngineThread::forEach(
                  [](EngineThread& t) { t.send<EM::ReloadCollection>(); });
              break;
            case IDC_IMG_NO_COVER_BROWSE:
              if (browseForImage(cfgImgNoCover, cfgImgNoCover))
                uSetDlgItemText(hWnd, IDC_IMG_NO_COVER, cfgImgNoCover);
              break;
            case IDC_IMG_LOADING_BROWSE:
              if (browseForImage(cfgImgLoading, cfgImgLoading))
                uSetDlgItemText(hWnd, IDC_IMG_LOADING, cfgImgLoading);
              break;
          }
        }
        break;
    }
    return FALSE;
  }
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
};

class BehaviourTab : public ConfigTab {
 public:
  CONFIG_TAB(BehaviourTab, "Behaviour", IDD_BEHAVIOUR_TAB);
  BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/) final {
    switch (uMsg) {
      case WM_INITDIALOG:
        loadConfig();
        loadActionList(IDC_DOUBLE_CLICK, cfgDoubleClick);
        loadActionList(IDC_MIDDLE_CLICK, cfgMiddleClick);
        loadActionList(IDC_ENTER_KEY, cfgEnterKey);
        SendDlgItemMessage(hWnd, IDC_FOLLOW_DELAY_SPINNER, UDM_SETRANGE32, 1, 999);
        SetDlgItemInt(hWnd, IDC_FOLLOW_DELAY, cfgCoverFollowDelay, 1);
        break;

      case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE) {
          textChanged(LOWORD(wParam));
          if (LOWORD(wParam) == IDC_FOLLOW_DELAY) {
            cfgCoverFollowDelay = std::clamp(
                int(uGetDlgItemInt(hWnd, IDC_FOLLOW_DELAY, nullptr, 1)), 1, 999);
          }
        } else if (HIWORD(wParam) == BN_CLICKED) {
          buttonClicked(LOWORD(wParam));
        } else if (HIWORD(wParam) == CBN_SELCHANGE) {
          listSelChanged(LOWORD(wParam));
        }
        break;
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
    uSendMessageText(list, CB_SELECTSTRING, 1, selectedItem);
    SetWindowRedraw(list, true);
  }
};

class DisplayTab : public ConfigTab {
 public:
  CONFIG_TAB(DisplayTab, "Display", IDD_DISPLAY_TAB);

  BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) final {
    switch (uMsg) {
      case WM_INITDIALOG:
        loadConfig();
        {
          int titlePosH = static_cast<int>(floor(cfgTitlePosH * 100 + 0.5));
          int titlePosV = static_cast<int>(floor(cfgTitlePosV * 100 + 0.5));
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
              hWnd, IDC_FONT_PREV, WM_SETTEXT, 0,
              reinterpret_cast<LPARAM>(cfgTitleFont.get_value().lfFaceName));

          uSendDlgItemMessage(
              hWnd, IDC_FRAME_WIDTH_SPIN, UDM_SETRANGE, 0, MAKELONG(short(30), short(0)));
          uSetDlgItemText(
              hWnd, IDC_FRAME_WIDTH, std::to_string(cfgHighlightWidth).data());
        }
        break;
      case WM_HSCROLL: {
        int titlePosH = uSendDlgItemMessage(hWnd, IDC_TPOS_H, TBM_GETPOS, 0, 0);
        int titlePosV = uSendDlgItemMessage(hWnd, IDC_TPOS_V, TBM_GETPOS, 0, 0);
        cfgTitlePosH = 0.01 * titlePosH;
        cfgTitlePosV = 0.01 * titlePosV;
        uSendDlgItemMessageText(
            hWnd, IDC_TPOS_H_P, WM_SETTEXT, 0, std::to_string(titlePosH).data());
        uSendDlgItemMessageText(
            hWnd, IDC_TPOS_V_P, WM_SETTEXT, 0, std::to_string(titlePosV).data());

        redrawMainWin();
      } break;
      case WM_DRAWITEM: {
        auto* drawStruct = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        HBRUSH brush{};
        switch (wParam) {
          case IDC_TEXTCOLOR_PREV:
            brush = CreateSolidBrush(cfgTitleColor);
            break;

          case IDC_BG_COLOR_PREV:
            brush = CreateSolidBrush(cfgPanelBg);
            break;
        }
        FillRect(drawStruct->hDC, &drawStruct->rcItem, brush);
      } break;

      case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE) {
          textChanged(LOWORD(wParam));

          switch (LOWORD(wParam)) {
            case IDC_ALBUM_FORMAT: {
              metadb_handle_ptr aTrack;
              if (metadb::get()->g_get_random_handle(aTrack)) {
                pfc::string8 preview;
                titleformat_object::ptr titleformat;
                titleformat_compiler::get()->compile_safe_ex(titleformat, cfgAlbumTitle);
                aTrack->format_title(nullptr, preview, titleformat, nullptr);
                preview.replace_string("\r\n", "↵");
                uSendDlgItemMessageText(hWnd, IDC_TITLE_PREVIEW, WM_SETTEXT, 0, preview);
              }
              redrawMainWin();
            } break;
            case IDC_FRAME_WIDTH: {
              pfc::string_fixed_t<16> highlightWidth;
              uGetDlgItemText(hWnd, IDC_FRAME_WIDTH, highlightWidth);
              cfgHighlightWidth = std::clamp(atoi(highlightWidth), 0, 30);
              redrawMainWin();
            } break;
          }
        } else if (HIWORD(wParam) == BN_CLICKED) {
          buttonClicked(LOWORD(wParam));
          switch (LOWORD(wParam)) {
            case IDC_APPLY_TITLE: {
              EngineThread::forEach(
                  [](EngineThread& t) { t.send<EM::ReloadCollection>(); });
            } break;
            case IDC_BG_COLOR: {
              COLORREF panelBg = cfgPanelBg;
              if (selectColor(panelBg)) {
                cfgPanelBg = panelBg;
                InvalidateRect(uGetDlgItem(hWnd, IDC_BG_COLOR_PREV), nullptr, TRUE);
              }
            } break;
            case IDC_TEXTCOLOR: {
              COLORREF titleColor = cfgTitleColor;
              if (selectColor(titleColor)) {
                cfgTitleColor = titleColor;
                InvalidateRect(uGetDlgItem(hWnd, IDC_TEXTCOLOR_PREV), nullptr, TRUE);
                EngineThread::forEach(
                    [](EngineThread& t) { t.send<EM::TextFormatChangedMessage>(); });
              }
            } break;
            case IDC_FONT: {
              LOGFONT titleFont = cfgTitleFont;
              if (selectFont(titleFont)) {
                cfgTitleFont = titleFont;
                uSendDlgItemMessage(
                    hWnd, IDC_FONT_PREV, WM_SETTEXT, 0,
                    reinterpret_cast<LPARAM>(cfgTitleFont.get_value().lfFaceName));
                EngineThread::forEach(
                    [](EngineThread& t) { t.send<EM::TextFormatChangedMessage>(); });
              }
            }
          }
          redrawMainWin();
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

class CoverTab : public ConfigTab {
  HFONT editBoxFont{};
  WNDPROC origEditboxProc = nullptr;

 public:
  CONFIG_TAB(CoverTab, "Cover Display", IDD_COVER_DISPLAY_TAB);

  BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/) final {
    switch (uMsg) {
      case WM_INITDIALOG:
        editBoxFont =
            CreateFont(-12, 0, 0, 0, 0, FALSE, 0, 0, 0, 0, 0, 0, 0, L"Courier New");
        loadConfig();
        loadConfigList();
        configSelectionChanged();
        setUpEditBox();
        return TRUE;

      case WM_DESTROY:
        DeleteObject(editBoxFont);

      case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE) {
          textChanged(LOWORD(wParam));
          if (LOWORD(wParam) == IDC_DISPLAY_CONFIG) {
            pfc::string8 script;
            uGetDlgItemText(hWnd, IDC_DISPLAY_CONFIG, script);
            cfgCoverConfigs[cfgCoverConfigSel.c_str()].script =
                linux_lineendings(script.c_str());
            uSetDlgItemText(hWnd, IDC_COMPILE_STATUS, "");
          }
        } else if (HIWORD(wParam) == BN_CLICKED) {
          buttonClicked(LOWORD(wParam));
          if (LOWORD(wParam) == IDC_SAVED_ADD) {
            addConfig();
          } else if (LOWORD(wParam) == IDC_SAVED_RENAME) {
            renameConfig();
          } else if (LOWORD(wParam) == IDC_SAVED_REMOVE) {
            removeConfig();
          } else if (LOWORD(wParam) == IDC_COMPILE) {
            compileConfig();
          }
        } else if (HIWORD(wParam) == CBN_SELCHANGE) {
          listSelChanged(LOWORD(wParam));
          if (LOWORD(wParam) == IDC_SAVED_SELECT) {
            configSelectionChanged();
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
      auto cInfo = make_shared<CompiledCPInfo>(compileCPScript(script));
      sessionCompiledCPInfo.set(cInfo);
      uSetDlgItemText(hWnd, IDC_COMPILE_STATUS, "Compilation successful");
      EngineThread::forEach(
          [&cInfo](EngineThread& t) { t.send<EM::ChangeCoverPositionsMessage>(cInfo); });
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
    if (cfgCoverConfigs.size() > 1) {
      pfc::string8 title;
      title << "Delete Config \"" << cfgCoverConfigSel << "\"";
      if (IDYES == uMessageBox(hWnd, "Are you sure?", title,
                               MB_APPLMODAL | MB_YESNO | MB_ICONQUESTION)) {
        auto i = cfgCoverConfigs.find(cfgCoverConfigSel.c_str());
        auto next = cfgCoverConfigs.erase(i);
        if (next == cfgCoverConfigs.end())
          --next;
        cfgCoverConfigSel = next->first.c_str();
        loadConfigList();
        configSelectionChanged();
      }
    }
  }
  void addConfig() {
    ConfigNameDialog dialog;
    if (dialog.query(hWnd)) {
      dialog.value.skip_trailing_char(' ');
      if (dialog.value.get_length()) {
        if (cfgCoverConfigs.count(dialog.value.c_str()) == 0) {
          auto script = builtInCoverConfigs()[defaultCoverConfig].script;
          auto name = dialog.value;
          cfgCoverConfigs.insert({name.c_str(), CoverConfig{script, false}});
          cfgCoverConfigSel = name;
          loadConfigList();
          configSelectionChanged();
        }
      }
    }
  }
  void renameConfig() {
    ConfigNameDialog dialog;
    if (dialog.query(hWnd, cfgCoverConfigSel)) {
      dialog.value.skip_trailing_char(' ');
      if (dialog.value.get_length()) {
        auto existing = cfgCoverConfigs.find(dialog.value.c_str());
        if (existing == cfgCoverConfigs.end() ||
            existing->first == cfgCoverConfigSel.c_str()) {
          auto node = cfgCoverConfigs.extract(cfgCoverConfigSel.c_str());
          if (node.empty())
            return;
          node.key() = dialog.value.c_str();
          cfgCoverConfigs.insert(std::move(node));
          cfgCoverConfigSel = dialog.value;
          loadConfigList();
          configSelectionChanged();
        }
      }
    }
  }
  void loadConfigList() {
    HWND list = GetDlgItem(hWnd, IDC_SAVED_SELECT);
    SetWindowRedraw(list, false);
    SendMessage(list, CB_RESETCONTENT, 0, 0);
    for (auto& [name, config] : cfgCoverConfigs) {
      uSendMessageText(list, CB_ADDSTRING, 0, name.c_str());
    }
    uSendMessageText(list, CB_SELECTSTRING, 1, cfgCoverConfigSel);
    SetWindowRedraw(list, true);
  }
  void configSelectionChanged() {
    try {
      const CoverConfig& config = cfgCoverConfigs.at(cfgCoverConfigSel.c_str());
      uSetDlgItemText(
          hWnd, IDC_DISPLAY_CONFIG, windows_lineendings(config.script).c_str());
      uSendDlgItemMessage(
          hWnd, IDC_DISPLAY_CONFIG, EM_SETREADONLY, static_cast<int>(config.buildIn), 0);
      uEnableWindow(
          uGetDlgItem(hWnd, IDC_SAVED_REMOVE), static_cast<BOOL>(!config.buildIn));
      uEnableWindow(
          uGetDlgItem(hWnd, IDC_SAVED_RENAME), static_cast<BOOL>(!config.buildIn));
    } catch (std::out_of_range&) {
    }
    uSetDlgItemText(hWnd, IDC_COMPILE_STATUS, "");
  }
};

using ListMap = std::unordered_map<int, std::string>;

ListMap multisamplingMap{
    {2, "  2"},
    {4, "  4"},
    {8, "  8"},
    {16, "16"},
};

static struct {
  int id;
  cfg_int* var;
  ListMap& map;
  // NOLINTNEXTLINE(cppcoreguidelines-interfaces-global-init)
} mappedListVarMap[] = {
    {IDC_MULTI_SAMPLING_PASSES, &cfgMultisamplingPasses, multisamplingMap},
};

class PerformanceTab : public ConfigTab {
 public:
  CONFIG_TAB(PerformanceTab, "Performance", IDD_PERF_TAB);
  BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/) final {
    switch (uMsg) {
      case WM_INITDIALOG:
        loadConfig();
        fillComboBoxes();

        SendDlgItemMessage(hWnd, IDC_CACHE_SIZE_SPIN, UDM_SETRANGE32, 2, 999);
        SetDlgItemInt(hWnd, IDC_CACHE_SIZE, cfgTextureCacheSize, 1);

        SendDlgItemMessage(hWnd, IDC_TEXTURE_SIZE_SPIN, UDM_SETRANGE32, 4, 2024);
        SetDlgItemInt(hWnd, IDC_TEXTURE_SIZE, cfgMaxTextureSize, 1);

        switch (cfgVSyncMode) {
          case VSYNC_SLEEP_ONLY:
            uButton_SetCheck(hWnd, IDC_VSYNC_OFF, true);
            break;
          case VSYNC_ONLY:
            uButton_SetCheck(hWnd, IDC_VSYNC_ONLY, true);
            break;
          case VSYNC_AND_SLEEP:
            uButton_SetCheck(hWnd, IDC_VSYNC_SLEEP, true);
            break;
        }
        break;

      case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE) {
          textChanged(LOWORD(wParam));

          if (LOWORD(wParam) == IDC_CACHE_SIZE) {
            cfgTextureCacheSize =
                std::clamp(int(uGetDlgItemInt(hWnd, IDC_CACHE_SIZE, nullptr, 1)), 2, 999);
          } else if (LOWORD(wParam) == IDC_TEXTURE_SIZE) {
            cfgMaxTextureSize = std::clamp(
                int(uGetDlgItemInt(hWnd, IDC_TEXTURE_SIZE, nullptr, 1)), 4, 2024);
          }
        } else if (HIWORD(wParam) == BN_CLICKED) {
          buttonClicked(LOWORD(wParam));
          switch (LOWORD(wParam)) {
            case IDC_VSYNC_OFF:
            case IDC_VSYNC_ONLY:
            case IDC_VSYNC_SLEEP:
              if (uButton_GetCheck(hWnd, IDC_VSYNC_OFF)) {
                cfgVSyncMode = VSYNC_SLEEP_ONLY;
              } else if (uButton_GetCheck(hWnd, IDC_VSYNC_ONLY)) {
                cfgVSyncMode = VSYNC_ONLY;
              } else if (uButton_GetCheck(hWnd, IDC_VSYNC_SLEEP)) {
                cfgVSyncMode = VSYNC_AND_SLEEP;
              }
          }
          redrawMainWin();
        } else if (HIWORD(wParam) == CBN_SELCHANGE) {
          comboBoxChanged(LOWORD(wParam));
          redrawMainWin();
        }
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
      uComboBox_GetText(uGetDlgItem(hWnd, comboBox), s, selected);
    } else {
      return;
    }
    for (auto& i : mappedListVarMap) {
      if (comboBox == i.id) {
        for (auto& [val, text] : i.map) {
          if (text == selected.c_str()) {
            *(i.var) = val;
            break;
          }
        }
      }
    }
  }
};

class ConfigWindow : public preferences_page {
 private:
  std::vector<unique_ptr<ConfigTab>> tabs;
  t_size currentTab;

 public:
  ConfigWindow() : currentTab(~0u) {}

  BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM /*wParam*/, LPARAM lParam) {
    switch (uMsg) {
      case WM_INITDIALOG: {
        HWND hWndTab = uGetDlgItem(hWnd, IDC_TABS);
        RECT rcTab;
        GetChildWindowRect(hWnd, IDC_TABS, &rcTab);
        uSendMessage(hWndTab, TCM_ADJUSTRECT, FALSE, reinterpret_cast<LPARAM>(&rcTab));

        tabs.emplace_back(make_unique<BehaviourTab>(hWnd));
        tabs.emplace_back(make_unique<SourcesTab>(hWnd));
        tabs.emplace_back(make_unique<DisplayTab>(hWnd));
        tabs.emplace_back(make_unique<CoverTab>(hWnd));
        tabs.emplace_back(make_unique<PerformanceTab>(hWnd));

        currentTab = sessionSelectedConfigTab;
        if (currentTab >= tabs.size())
          currentTab = 0;
        uSendMessage(hWndTab, TCM_SETCURSEL, currentTab, 0);
        tabs[currentTab]->show();
      } break;

      case WM_NCDESTROY: {
        sessionSelectedConfigTab = currentTab;
        tabs.clear();
      } break;

      case WM_NOTIFY:
        NMHDR& msg = *reinterpret_cast<LPNMHDR>(lParam);
        if (msg.idFrom == IDC_TABS) {
          if (msg.code == TCN_SELCHANGE) {
            if (currentTab < tabs.size())
              tabs[currentTab]->hide();
            currentTab = SendDlgItemMessage(hWnd, IDC_TABS, TCM_GETCURSEL, 0, 0);
            if (currentTab < tabs.size())
              tabs[currentTab]->show();
          }
        }
        break;
    }
    return FALSE;
  }

  static BOOL CALLBACK dialogProxy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ConfigWindow* configWindow = nullptr;
    if (uMsg == WM_INITDIALOG) {
      configWindow = reinterpret_cast<ConfigWindow*>(lParam);
      SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(configWindow));
    } else {
      configWindow =
          reinterpret_cast<ConfigWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }
    return configWindow->dialogProc(hWnd, uMsg, wParam, lParam);
  }

  HWND create(HWND parent) final {
    return uCreateDialog(
        IDD_CONFIG_TABS, parent, dialogProxy, reinterpret_cast<LPARAM>(this));
  }

  const char* get_name() final { return "Chronflow"; }

  GUID get_guid() final { return guid_configWindow; }

  GUID get_parent_guid() final { return preferences_page::guid_display; }

  bool reset_query() final { return false; }
  void reset() final {}
};

static service_factory_single_t<ConfigWindow> x_configWindow;
