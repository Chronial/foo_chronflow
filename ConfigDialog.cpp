#include "ConfigDialog.h"
// clang-format off
#include "EngineThread.h" //(1)
#include "ConfigWindow.h" //(2)
#include "Engine.h"
// clang-format on
#include "ConfigData.h"
#include "dbAlbumInfo.h"
#include "yesno_dialog.h"

namespace coverflow {

static library_viewer_factory_t<LibraryViewer> g_libraryViewer;

using ::engine::EngineThread;

enum ChangedFlags {
  BACT_RELOAD = 1 << 0,   // 1 Reload maybe required
  BACT_REDRAW = 1 << 1,   // 2 Redraw   "      "
  //BACT_UNDEFINED = 1 << 7 // 128 Other
};

void ConfigDialog::BindControls(UINT_PTR ndx, HWND hWndTab, int cmd) {
  bindings_.Clear(); //safeguard
  switch (ndx) {
    case IDD_BEHAVIOUR_TAB:
      bindings_.Bind(configData->FindAsYouType, 0, hWndTab, IDC_FIND_AS_YOU_TYPE);
      bindings_.Bind(configData->FindAsYouTypeCaseSens, 0, hWndTab, IDC_FIND_AS_YOU_TYPE_CS);
      bindings_.Bind(configData->CoverFollowsPlayback, 0, hWndTab, IDC_FOLLOW_PLAYBACK);
      bindings_.Bind(configData->CoverFollowDelay, 0, hWndTab, IDC_FOLLOW_DELAY);
      bindings_.Bind(configData->TargetPlaylist, 0, hWndTab, IDC_TARGET_PL);
      bindings_.Bind(configData->DoubleClick, 0, hWndTab, IDC_DOUBLE_CLICK);
      bindings_.Bind(configData->MiddleClick, 0, hWndTab, IDC_MIDDLE_CLICK);
      bindings_.Bind(configData->EnterKey, 0, hWndTab, IDC_ENTER_KEY);
      bindings_.Bind(configData->CustomActionFlag, 0, hWndTab, IDC_HIDDEN_CUSTOM_ACTION_FLAG);
      bindings_.Bind(configData->CustomActionHRate, 0, hWndTab, IDC_EDIT_BEHA_HRATE);
      bindings_.Bind(configData->CoverFollowsLibrarySelection, 0, hWndTab, IDC_CHECK_BEHA_COVER_FOLLOWS_LIBRARY);
      bindings_.Bind(configData->CoverFollowsAnonymSelection, 0, hWndTab, IDC_CHECK_BEHA_COVER_FOLLOWS_ANONYM);

      break;
    case IDD_SOURCE_TAB:
      bindings_.Bind(configData->Filter, BACT_RELOAD, hWndTab, IDC_FILTER);
      bindings_.Bind(configData->Group, BACT_RELOAD, hWndTab, IDC_GROUP);
      bindings_.Bind(configData->SortGroup, BACT_RELOAD, hWndTab, IDC_SORT_GROUP);
      bindings_.Bind(configData->Sort, BACT_RELOAD, hWndTab, IDC_SORT);
      bindings_.Bind(configData->InnerSort, BACT_RELOAD, hWndTab, IDC_INNER_SORT);

      bindings_.Bind(configData->SourcePlaylist, BACT_RELOAD, hWndTab, IDC_SOURCE_FROM_PLAYLIST);
      bindings_.Bind(configData->SourceActivePlaylist, BACT_RELOAD, hWndTab, IDC_SOURCE_FROM_ACTIVE_PLAYLIST);
      bindings_.Bind(configData->SourcePlaylistName, BACT_RELOAD, hWndTab, IDC_COMBO_SOURCE_PLAYLIST_NAME);
      bindings_.Bind(configData->SourcePlaylistGroup, BACT_RELOAD, hWndTab, IDC_SOURCE_PLAYLIST_GROUP);
      bindings_.Bind(configData->SourcePlaylistNGTitle, BACT_RELOAD, hWndTab, IDC_SOURCE_PLAYLIST_NGTITLE);
      bindings_.Bind(configData->CoverFollowsPlaylistSelection, 0, hWndTab, IDC_CHECK_BEHA_COVER_FOLLOWS_PLAYLIST);
      bindings_.Bind(configData->CoverHighLightPlaylistSelection, 0, hWndTab, IDC_CHECK_BEHA_COVER_HIGHLIGHT_PLAYLIST);
      bindings_.Bind(configData->SourceLibrarySelector, 0, hWndTab, IDC_SOURCE_FROM_LIBRARY_SELECTOR);
      bindings_.Bind(configData->SourceLibrarySelectorLock, 0, hWndTab, IDC_SOURCE_FROM_LIBRARY_SELECTOR_LOCK);
      break;
    case IDD_DISPLAY_TAB:
      bindings_.Bind(configData->ShowAlbumTitle, BACT_REDRAW, hWndTab, IDC_ALBUM_TITLE);
      bindings_.Bind(configData->AlbumTitle, BACT_RELOAD, hWndTab, IDC_ALBUM_FORMAT);
      bindings_.Bind(configData->TitleColorCustom, BACT_REDRAW, hWndTab, IDC_TEXTCOLOR_CUSTOM);
      bindings_.Bind(configData->TitleColor, BACT_REDRAW, hWndTab, IDC_HIDDEN_TITLE_COLOR_CUSTOM);
      bindings_.Bind(configData->PanelBgCustom, BACT_REDRAW, hWndTab, IDC_BG_COLOR_CUSTOM);
      bindings_.Bind(configData->PanelBg, BACT_REDRAW, hWndTab, IDC_HIDDEN_PANELBG_CUSTOM);
      bindings_.Bind(configData->TitleFontCustom, BACT_REDRAW, hWndTab, IDC_FONT_CUSTOM);
      bindings_.Bind(configData->TitleFont, BACT_REDRAW, hWndTab, IDC_HIDDEN_LOGFONT);
      bindings_.Bind(configData->HighlightWidth, BACT_REDRAW, hWndTab, IDC_FRAME_WIDTH);
      bindings_.Bind(configData->TitlePosV, BACT_REDRAW, hWndTab, IDC_HIDDEN_TPOS_V);
      bindings_.Bind(configData->TitlePosH, BACT_REDRAW, hWndTab, IDC_HIDDEN_TPOS_H);
      bindings_.Bind(configData->ImgNoCover, BACT_RELOAD, hWndTab, IDC_IMG_NO_COVER);
      bindings_.Bind(configData->ImgLoading, BACT_RELOAD, hWndTab, IDC_IMG_LOADING);
      bindings_.Bind(configData->CustomCoverFrontArt, BACT_RELOAD, hWndTab, IDC_HIDDEN_DISPLAY_CUSTOM_COVER_ART);
      bindings_.Bind(configData->CoverArtEnablePngAlpha, BACT_RELOAD, hWndTab, IDC_COVER_ART_PNG8_ALPHA);
      bindings_.Bind(configData->DisplayExtViewerPath, BACT_REDRAW, hWndTab, IDC_HIDDEN_EXT_VIEWER_PATH);
      bindings_.Bind(configData->CoverUseLegacyExternalViewer, BACT_REDRAW, hWndTab, IDC_COVER_DISPLAY_LEGACY_EXTVIEWER);
      bindings_.Bind(configData->DisplayFlag, BACT_REDRAW, hWndTab, IDC_HIDDEN_DISPLAY_FLAG);
      bindings_.Bind(configData->DisplayArtFilterFlag, BACT_REDRAW, hWndTab, IDC_HIDDEN_DISPLAY_EXT_ARTFILTER_FLAG);
      break;
    case IDD_COVER_DISPLAY_TAB:
      //bindings_.Bind(configData->CoverConfigSel, 0, hWndTab, IDC_HIDDEN_SAVED_SELECT);
      bindings_.Bind(configData->CoverConfigSel, 0, hWndTab, IDC_SAVED_SELECT);
      bindings_.Bind(configData->CoverConfigs, 0, hWndTab, IDC_DISPLAY_CONFIG,
                     configData->CoverConfigSel);
      break;
    case IDD_PERF_TAB:
      bindings_.Bind(configData->Multisampling, 0, hWndTab, IDC_MULTI_SAMPLING);
      bindings_.Bind(configData->MultisamplingPasses, 0, hWndTab, IDC_MULTI_SAMPLING_PASSES);
      bindings_.Bind(configData->TextureCacheSize, 0, hWndTab, IDC_CACHE_SIZE);
      bindings_.Bind(configData->MaxTextureSize, 0, hWndTab, IDC_TEXTURE_SIZE);
      bindings_.Bind(configData->TextureCompression, 0, hWndTab, IDC_TEXTURE_COMPRESSION);
      bindings_.Bind(configData->EmptyCacheOnMinimize, 0, hWndTab, IDC_EMPTY_ON_MINIMIZE);
      bindings_.Bind(configData->VSyncMode, 0, hWndTab, IDC_HIDDEN_VSYNC_MODE);
      bindings_.Bind(configData->ShowFps, 0, hWndTab, IDC_SHOW_FPS);
      break;
    case IDD_CONTEXT_MENU_TAB:
      bindings_.Bind(configData->CtxShowPlaylistMenu, 0, hWndTab, IDC_CTX_SOURCE_HIDE);
      bindings_.Bind(configData->CtxShowDisplayMenu, 0, hWndTab, IDC_CTX_DISPLAY_HIDE);
      bindings_.Bind(configData->CtxShowSelectorMenu, 0, hWndTab, IDC_CTX_SELECTOR_HIDE);
      bindings_.Bind(configData->CtxShowExtViewerMenu, 0, hWndTab, IDC_CTX_EXTERNALVIEWER_HIDE);
      bindings_.Bind(configData->CtxShowActionsMenu, 0, hWndTab, IDC_CTX_CUSTOMACTIONS_HIDE);
      //bindings_.Bind(configData->SourceHideExpMenu, 0, hWndTab, IDC_CTX_PREFS_HIDE);
      break;
  }
}

void ConfigDialog::ClearBindings() {
  bindings_.Clear();
}

t_uint32 ConfigDialog::get_state() {
  t_uint32 state = preferences_state::resettable;
  if (HasChanged())
    state |= preferences_state::changed;
  return state | preferences_state::dark_mode_supported;
}

void ConfigDialog::apply() {
  HWND wndTab = m_configwindow->GetCurrentTabWnd();

  //prepare for possible display config change
  //todo: this looks ugly, rework it, maybe better be done by container
  CoverConfigMap scriptMap;
  scriptMap.clear();
  pfc::string8 lastCoverSelection, lastCPScript;
  bool b_check_display = bindings_.GetFVal(&scriptMap, wndTab, IDC_DISPLAY_CONFIG);
  if (b_check_display) {
    lastCoverSelection = configData->CoverConfigSel.c_str();
    lastCPScript = GetCoverConfigScript(configData->CoverConfigs, configData->CoverConfigSel);
  }

  //apply bindings to configData
  bindings_.FlowToVar(wndTab);
  //OnChanged();

  //apply new session cover config if required
  if (b_check_display) {
    pfc::string8 currentCPScript = GetCoverConfigScript(configData->CoverConfigs,
        configData->CoverConfigSel);
    bool bscriptmod = !(lastCPScript.equals(currentCPScript));
    try {
      std::pair<int, std::shared_ptr<CompiledCPInfo>> cInfo;
      if (bscriptmod) {
        cInfo.first = configData->GetCCPosition(configData->CoverConfigSel);
        cInfo.second = make_shared<CompiledCPInfo>(compileCPScript(currentCPScript));
      } else
        cInfo = configData->sessionCompiledCPInfo.get();

      if (!lastCoverSelection.equals(configData->CoverConfigSel) || bscriptmod) {
        EngineThread::forEach([&cInfo](EngineThread& t) {
          t.send<EM::ChangeCoverPositionsMessage>(cInfo.second, (LPARAM)NULL);
        });
        configData->sessionCompiledCPInfo.set(cInfo.first, cInfo.second);
      }
    } catch (std::exception&) {
      //..
    }
  }

  if (bneedreload) {
    EngineThread::forEach([](EngineThread& t)
    {
        t.send<EM::ReloadCollection>(NULL);
    });
  } else {
  if (bneedredraw) {
      EngineThread::forEach([](auto& t) { t.invalidateWindow(); });
    }
  }
}

void ConfigDialog::reset() {

  if (m_configwindow == nullptr)
    return;

  pfc::string8 title = "Reset Component";
  if (IDYES != uMessageBox(get_wnd(),
      "Resetting ALL values to the component default configuration, are you sure?",
      title, MB_APPLMODAL | MB_YESNO | MB_ICONQUESTION)) {
    return;
  }

  configData->Reset();
  bindings_.Clear();
  ConfigTab* configTab = m_configwindow->GetCurrentConfigTab();
  HWND wndTab = m_configwindow->GetCurrentTabWnd();
  configTab->dialogProc(wndTab, WM_INITDIALOG, (WPARAM)nullptr, (LPARAM)nullptr);
  //OnChanged();
  m_callback->on_state_changed();
};

bool ConfigDialog::HasChanged() {
  std::pair<int, bool> pres = bindings_.HasChangedExt();
  bneedreload = (pres.first & BACT_RELOAD);
  bneedredraw = (pres.first & BACT_REDRAW);
  //todo: bneedGLRebuild
  return pres.second;
}

void ConfigDialog::OnChanged() {
  // notify host that state changed
  // will enable/disable apply button
  m_callback->on_state_changed();
}

BOOL ConfigDialog::OnInitDialog(CWindow wndFocus, LPARAM lInitParam) {
  m_hWnd = get_wnd();
  m_configwindow = new ConfigWindow(get_wnd());

  if (m_configwindow != nullptr) {
    m_configwindow->OnInit(get_wnd(), lInitParam);
  }
  //Dark mode
  AddDialog(m_hWnd);
  AddControls(m_hWnd);

  if (m_configwindow != nullptr) {
    m_configwindow->OnInit(get_wnd(), lInitParam);
  }

  return FALSE;
}

LRESULT ConfigDialog::OnNotify(int idCtrl, LPNMHDR pnmh) {
  NMHDR& msg = *reinterpret_cast<LPNMHDR>(pnmh);
  HWND wndFrom = pnmh->hwndFrom;
  UINT_PTR idFrom = pnmh->idFrom;

  switch (idCtrl) {
    case IDC_TABS:
      switch (pnmh->code) {
        case TCN_SELCHANGING: {        
          if (get_state() & preferences_state::changed) {
            pfc::string8 title_yn(m_configwindow->GetCurrentConfigTab()->getTabTitle());
            title_yn << " configuration";
            CYesNoApiDialog yndlg;
            auto res = yndlg.query(get_wnd(), {title_yn, "Apply Changes ?"}, true, false);
            switch (res) {
              case 1 /*yes*/:
                apply();
                break;
              case 2 /*no*/:
                // do nothing
                bdiscarding = true;
                break;
              default /*cancel*/:
                return TRUE;
            }
          }
          LoadingBindings(true);

          m_configwindow->LoadDisableMap();
          ClearBindings();
          LoadingBindings(false);
          break;
        }
        case TCN_SELCHANGE:
          //call will trigger wm_dialog_init->notifyinit->CF_USER_CONFIG_NEWTAB
          m_configwindow->ShowTab(idCtrl, pnmh);
          break;
        case CF_USER_CONFIG_CHANGE:
          wndFrom = pnmh->hwndFrom;
          idFrom = pnmh->idFrom; //the ctrl UINT

          //refresh apply button state
          if (!IsLoadingBindings())
            //OnChanged();
            m_callback->on_state_changed();
          break;

        case CF_USER_CONFIG_NEWTAB:
          wndFrom = pnmh->hwndFrom;
          idFrom = pnmh->idFrom; //the tab UINT
          //Dark mode
          AddDialogWithControls(wndFrom);
          LoadingBindings(true);
          BindControls(pnmh->idFrom, pnmh->hwndFrom, 1);
          bindings_.FlowToControl(pnmh->hwndFrom);

          if (bdiscarding) {
            //OnChanged();
            m_callback->on_state_changed();
            bdiscarding = false;
          }
          m_configwindow->LoadDisableMap();
          LoadingBindings(false);
          break;
        default:
          break;
      }
      break; //end IDC_TABS
  }
  return FALSE;
}

void ConfigDialog::OnDestroy() {
  //ConfigWindow saves session tab index
  if (m_configwindow != nullptr)
    m_configwindow->OnDestroy();
}
} // namespace coverflow
