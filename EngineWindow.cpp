// clang-format off
#include "EngineThread.h"  //(1)
#include "EngineWindow.h"  //(2)
#include "ContainerWindow.h"
#include "Engine.h"
// clang-format on

#include "MyActions.h"
#include "lib/win32_helpers.h"
#include "ConfigContextPlaylist.h"
#include "ConfigContextDisplay.h"
#include "ConfigContextSelector.h"
#include "TrackDropSource.h"

//#include "PlaybackTracer.h"
//#include "SDK\library_manager.h"

namespace engine {

  using namespace coverflow;
  using EM = engine::Engine::Messages;

  int GLFWContext::count = 0;

  GLFWContext::GLFWContext() {
    if (count == 0) {
    glfwSetErrorCallback([](int error, const char* description) {
        throw std::runtime_error(PFC_string_formatter() << "glfw error: " << description);
    });
    TRACK_CALL_TEXT("glfwInit");
    TRACK_CODE("ensure_main_thread()", core_api::ensure_main_thread());
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize glfw");
    }
    }
    ++count;
  }
  GLFWContext::~GLFWContext() {
    --count;
    if (count == 0) {
      TRACK_CALL_TEXT("glfwTerminate");
      TRACK_CODE("ensure_main_thread()", core_api::ensure_main_thread());
      glfwTerminate();
    }
  }
  void EngineWindow::createWindow() {
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_DECORATED, FALSE);
    glfwWindowHint(
        GLFW_SAMPLES, configData->Multisampling ? configData->MultisamplingPasses : 0);
    glfwWindowHint(GLFW_FOCUSED, FALSE);
    glfwWindowHint(GLFW_RESIZABLE, FALSE);
    glfwWindowHint(GLFW_VISIBLE, FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    glfwWindow.reset(glfwCreateWindow(
      640, 480, PFC_string_formatter() << AppNameInternal << " render window", nullptr,
      nullptr));
    if (!glfwWindow) {
      throw std::runtime_error("Unknown error while creating opengl window");
    }
    hWnd = glfwGetWin32Window(glfwWindow.get());

    WIN32_OP_D(SetParent(hWnd, container.getHWND()));
    const LONG nNewStyle = (GetWindowLong(hWnd, GWL_STYLE) & ~WS_POPUP) | WS_CHILDWINDOW;
    SetWindowLong(hWnd, GWL_STYLE, nNewStyle);
    const ULONG_PTR cNewStyle = GetClassLongPtr(hWnd, GCL_STYLE) | CS_DBLCLKS;
    WIN32_OP_D(SetClassLongPtr(hWnd, GCL_STYLE, cNewStyle));
    WIN32_OP_D(SetWindowSubclass(hWnd,
      WINLAMBDA([](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR,
        DWORD_PTR dwRefData) noexcept {
        try {
          return reinterpret_cast<EngineWindow*>(dwRefData)->messageHandler(
            uMsg, wParam, lParam);
        }
        catch (std::exception & e) {
          FB2K_console_formatter() << "Exception in " << AppNameInternal
            << " EngineWindow MessageHandler: " << e.what();
          return DefSubclassProc(hwnd, uMsg, wParam, lParam);
        }
     }), 0, reinterpret_cast<DWORD_PTR>(this)));

    static auto wrap = [](auto f) noexcept {
      try {
        f();
      }
      catch (std::exception & e) {
        FB2K_console_formatter() << "Exception in " << AppNameInternal << " EngineWindow event handler: " << e.what();
      }
    };
    glfwSetWindowUserPointer(glfwWindow.get(), this);
    glfwSetScrollCallback(glfwWindow.get(), [](GLFWwindow* window, double xoffset, double yoffset) {
      wrap([&] {
        static_cast<EngineWindow*>(glfwGetWindowUserPointer(window))
          ->onScroll(xoffset, yoffset);
      });
    });
    glfwSetWindowRefreshCallback(glfwWindow.get(), [](GLFWwindow* window) {
      wrap([&] {
        static_cast<EngineWindow*>(glfwGetWindowUserPointer(window))->onDamage();
      });
    });
    glfwSetWindowSizeCallback(
    glfwWindow.get(), [](GLFWwindow* window, int width, int height) {
      wrap([&] {
        static_cast<EngineWindow*>(glfwGetWindowUserPointer(window))
            ->onWindowSize(width, height);
      });
    });
}
void EngineWindow::setWindowSize(int width, int height) {
  glfwSetWindowSize(glfwWindow.get(), width, height);
}
void EngineWindow::makeContextCurrent() {
  glfwMakeContextCurrent(glfwWindow.get());
}
void EngineWindow::swapBuffers() {
  glfwSwapBuffers(glfwWindow.get());
}

LRESULT EngineWindow::messageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_MOUSEACTIVATE:
      SetFocus(hWnd);
      return MA_ACTIVATE;
    case WM_SETFOCUS: {
      selectionHolder = ui_selection_manager_v2::get()->acquire();
      //setSelection(selection, true);
      break;
    }
    case WM_KILLFOCUS: {
      selectionHolder.release();
      break;
    }
    case WM_GETDLGCODE:
      return DLGC_WANTALLKEYS;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
      onMouseClick(uMsg, wParam, lParam);
      return 0;
    case WM_RBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_NCRBUTTONUP:
      // Generate WM_CONTEXTMENU messages
      return DefWindowProc(hWnd, uMsg, wParam, lParam);
    case WM_CONTEXTMENU:
      if (defaultUiCallback.is_valid() && defaultUiCallback->is_edit_mode_enabled()) {
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
      }
      else {
        onContextMenu(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
      }
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
      if (onKeyDown(uMsg, wParam, lParam))
       return 0;
       break;
    case WM_CHAR:
      if (onChar(wParam))
        return 0;
      break;
    case WM_MYACTIONS_SET_DISPLAY:
      cmdActivateVisualization(lParam);
      break;
    //todo: remove all actions in playlist mode
    case WM_MYACTIONS_CANCELED:
      // MessageBeep(MB_ICONINFORMATION);
      engineThread->send<EM::VisualErrorMessage>();
      break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
bool EngineWindow::onChar(WPARAM wParam) {
  //todo: check compatibility with playlist mode
  if (configData->FindAsYouType) {
    engineThread->send<EM::CharEntered>(wParam);
    return true;
  }
  else {
    return false;
  }
}
void EngineWindow::onMouseClick(UINT uMsg, WPARAM /*wParam*/, LPARAM lParam) {
  auto future = engineThread->sendSync<EM::GetAlbumAtCoords>(
  GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
  auto clickedAlbum = future.get();
  if (!clickedAlbum)
    return;
  if (uMsg == WM_LBUTTONDOWN) {
    POINT pt;
    GetCursorPos(&pt);
    if (DragDetect(hWnd, pt)) {
      doDragStart(clickedAlbum.value());
      return;
    }
  }
  onClickOnAlbum(clickedAlbum.value(), uMsg);
}
void EngineWindow::doDragStart(const AlbumInfo& album) {
  static_api_ptr_t<playlist_incoming_item_filter> piif;
  pfc::com_ptr_t<IDataObject> pDataObject = piif->create_dataobject_ex(album.tracks);
  pfc::com_ptr_t<IDropSource> pDropSource = TrackDropSource::g_create(hWnd);

  DWORD effect;
  DoDragDrop(pDataObject.get_ptr(), pDropSource.get_ptr(), DROPEFFECT_COPY, &effect);
}
void EngineWindow::onClickOnAlbum(const AlbumInfo& album, UINT uMsg) {
  if (uMsg == WM_LBUTTONDOWN) {
    engineThread->send<EM::MoveToAlbumMessage>(album, true);
  }
  else if (uMsg == WM_MBUTTONDOWN) {
    if (!configData->IsWholeLibrary()) return;
    executeAction(configData->MiddleClick, album, this->hWnd,
    ActionGetBlockFlag(configData->CustomActionFlag, AB_MIDDLECLICK));
  }
  else if (uMsg == WM_LBUTTONDBLCLK) {
    if (!configData->IsWholeLibrary()) {
      cmdPlaylistSourcePlay(album);
    } else {
      executeAction(configData->DoubleClick, album, this->hWnd,
      ActionGetBlockFlag(configData->CustomActionFlag, AB_DOUBLECLICK));
    }
  }
}
bool EngineWindow::onKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  if (wParam == VK_RETURN && ((GetKeyState(VK_CONTROL) & 0x8000))) {
    //...
  }
  else if (wParam == VK_RETURN) {
    auto targetAlbum = engineThread->sendSync<EM::GetTargetAlbum>().get();
    if (targetAlbum) {
      if (!configData->IsWholeLibrary()) {
        cmdPlaylistSourcePlay(targetAlbum.value());
      } else {
        executeAction(configData->EnterKey, targetAlbum.value(), this->hWnd,
            ActionGetBlockFlag(configData->CustomActionFlag, AB_ENTER));
      }
    }
    return true;
  }
  else if (wParam == VK_F2) {
    configData->FindAsYouTypeCaseSens = !configData->FindAsYouTypeCaseSens;
    return true;
  }
  else if (wParam == VK_F4) {

    if (!configData->IsWholeLibrary()) {

      src_state srcstate;
      configData->GetState(srcstate);
      srcstate.wholelib.second = srcstate.wholelib.first;
      srcstate.grouped.second = !srcstate.grouped.first;

      //todo: clean this
      if (configData->SourcePlaylistGroup) {

        engineThread->send<EM::SourceChangeMessage>(srcstate);

        configData->SourcePlaylistGroup = !configData->SourcePlaylistGroup;

        engineThread->send<EM::ReloadCollection>();
      }
      else {

        //todo: flickers when reassigned, revise parameters
        engineThread->send<EM::SourceChangeMessage>(srcstate);

        configData->SourcePlaylistGroup = !configData->SourcePlaylistGroup;

        engineThread->send<EM::ReloadCollection>();
      }
    }
    return true;
  }
  else if (wParam == VK_F5) {
    engineThread->send<EM::ReloadCollection>();
    return true;
  }
  else if (wParam == VK_F6) {
    engineThread->send<EM::MoveToNowPlayingMessage>();
    return true;
  }
  else if (wParam == VK_F8) {
    if (GetKeyState(VK_CONTROL) & 0x8000) {
      if (!configData->CtxHideSelectorMenu)
        cmdToggleLibraryFilterSelectorSource(true);
    } else
    // assign the current playlist as album source
    cmdAssignPlaylistSource();
  }
  else if (wParam == VK_F9) {
    // toggle fixed playlist source mode on/off
    cmdTogglePlaylistSource();
    return true;
  }
  else if (wParam == VK_F10) {
    if (GetKeyState(VK_CONTROL) & 0x8000) {
      if (!configData->CtxHideSelectorMenu)
        cmdToggleLibraryFilterSelectorSource(false);
    } else
    // toggle active playlist is the source on/off
    cmdToggleActivePlaylistSource();
    return true;
  }
  else if (wParam == VK_RIGHT || wParam == VK_LEFT || wParam == VK_NEXT ||
    wParam == VK_PRIOR) {
    int move = 0;
    if (wParam == VK_RIGHT) {
        move = +1;
    }
    else if (wParam == VK_LEFT) {
        move = -1;
    }
    else if (wParam == VK_NEXT) {
        move = +10;
    }
    else if (wParam == VK_PRIOR) {
        move = -10;
    }
    move *= LOWORD(lParam);
    engineThread->send<EM::MoveTargetMessage>(move, false);
    return true;
  }
  else if (wParam == VK_UP || wParam == VK_DOWN) {
    engineThread->send<EM::ArtChangedMessage>(wParam == VK_DOWN,
      (GetKeyState(VK_CONTROL) & 0x8000));
    return true;
  }
  else if (wParam == VK_HOME) {
    engineThread->send<EM::MoveTargetMessage>(-1, true);
    return true;
  }
  else if (wParam == VK_END) {
    engineThread->send<EM::MoveTargetMessage>(1, true);
    return true;
  }
  else if ((wParam >= '0' && wParam <= '9') && GetKeyState(VK_CONTROL) & 0x8000) {
    //0 default, 1 first script...
    int ndx = wParam - 0x30;
    if (ndx == 0)
      ndx = configData->GetCCPosition();
    else
      ndx--;
    cmdActivateVisualization(ndx);
    return true;
  }
  else if (!(configData->FindAsYouType &&  // disable hotkeys that interfere with
                                              // find-as-you-type
      (uMsg == WM_KEYDOWN) &&
      ((wParam > 'A' && wParam < 'Z') || (wParam > '0' && wParam < '9') ||
      (wParam == ' ')) &&
          ((GetKeyState(VK_CONTROL) & 0x8000) == 0))) {
      auto targetAlbum = engineThread->sendSync<EM::GetTargetAlbum>().get();
      static_api_ptr_t<keyboard_shortcut_manager> ksm;
      if (targetAlbum) {
          return ksm->on_keydown_auto_context(
              targetAlbum->tracks, wParam, contextmenu_item::caller_media_library_viewer);
      }
      else {
          return ksm->on_keydown_auto(wParam);
      }
  }
  return false;
}

//set or toggle SourcePlaylist and SourcePlaylistName
//returns true if display should be updated
//disables active playlist
bool ConfigPlaylistSource(bool toggle, bool reassign) {

  bool prevstate = configData->SourcePlaylist;

  if (toggle)
    configData->SourcePlaylist = !configData->SourcePlaylist;

  if (configData->SourcePlaylist && reassign) {
      static_api_ptr_t<playlist_manager> pm;
      pm->activeplaylist_get_name(configData->SourcePlaylistName);
  }
  bool bmirror = configData->IsSourceOnAndMirrored(true);

  //in all cases assume turning ActivePlaylist off
  configData->SourceActivePlaylist = false;

  return configData->SourcePlaylist != prevstate || (configData->SourcePlaylist == prevstate && !bmirror);
}

bool ToggleActivePlaylistMode() {
  bool breload = false;
  configData->SourceActivePlaylist = !configData->SourceActivePlaylist;

  if (configData->SourceActivePlaylist) {
    static_api_ptr_t<playlist_manager> pm;
    t_size activepl = pm->get_active_playlist();

    if (activepl == pfc_infinite) {
      return false;
    }

    //is SourcePlaylist on and in the same playlist?
    breload = !configData->IsSourcePlaylistOn(activepl, 1);

    pm->activeplaylist_get_name(configData->SourceActivePlaylistName);
    configData->SourcePlaylist = true;
  }
  else {
    configData->SourcePlaylist = false;
    breload = true;
  }
  return breload;
}

void EngineWindow::cmdToggleActivePlaylistSource() {

  configData->SourceLibrarySelector = false;
  configData->SourceLibrarySelectorLock = false;

  src_state srcstate;
  configData->GetState(srcstate);

  bool brefresh = ToggleActivePlaylistMode();

  configData->GetState(srcstate, false);

  if (brefresh) {

    engineThread->send<EM::SourceChangeMessage>(srcstate);
    engineThread->send<EM::ReloadCollection>();
  }
}

void EngineWindow::cmdAssignPlaylistSource() {

  configData->SourceLibrarySelector = false;
  configData->SourceLibrarySelectorLock = false;

  src_state srcstate;
  configData->GetState(srcstate);

  bool brefresh = ConfigPlaylistSource(!configData->SourcePlaylist, true);

  configData->GetState(srcstate, false);

  if (brefresh) {
    engineThread->send<EM::SourceChangeMessage>(srcstate);
    engineThread->send<EM::ReloadCollection>();
  }
}

void EngineWindow::cmdTogglePlaylistSource() {

  configData->SourceLibrarySelector = false;
  configData->SourceLibrarySelectorLock = false;

  src_state srcstate;
  configData->GetState(srcstate);
  //because in active playlist mode playlist mode is also turned on
  bool breload = ConfigPlaylistSource(!configData->SourceActivePlaylist, false);

  configData->GetState(srcstate, false);

  if (breload) {
    engineThread->send<EM::SourceChangeMessage>(srcstate);
    engineThread->send<EM::ReloadCollection>();
  }
}

t_size FindItemFromPos(metadb_handle_list list, metadb_handle_ptr item, t_size pos) {

  for (int i = pos; i < list.get_count(); i++) {
    bool bcomp = list.get_item(i) == item.get_ptr();
    if (bcomp) {
      return i;
    }
  }
  return pfc_infinite;
}

t_size GetHighlightMask(t_size playlist, const AlbumInfo& album, bit_array_bittable& iomask) {
  static_api_ptr_t<playlist_manager> pm;
  metadb_handle_list plist;
  pm->playlist_get_all_items(playlist, plist);
  iomask.resize(plist.get_count());

  t_size retpos = pfc_infinite;
  if (!configData->IsPlaylistSourceModeUngrouped()) {
    for (int j = 0; j < plist.get_count(); j++) {
      int res = FindItemFromPos(album.tracks, plist.get_item(j), 0);
      if (res != pfc_infinite)
        retpos = retpos == pfc_infinite ? j : retpos;
      iomask.set(j, res != pfc_infinite);
    }
  } else {
    std::string strkey = album.pos.key;
    t_size ndxpos = strkey.rfind("|", strkey.length());
    ndxpos++;
    std::string ndxstr = strkey.substr(ndxpos, strkey.length() - (ndxpos));
    retpos = atoi(ndxstr.c_str());
    iomask.set(retpos, true);
  }
  return retpos;
}
void EngineWindow::cmdPlaylistSourcePlay(const AlbumInfo& album) {

  static_api_ptr_t<playlist_manager> pm;
  t_size playlist = pm->find_playlist(configData->InSourePlaylistGetName());
  bit_array_bittable hlmask;
  t_size first = GetHighlightMask(playlist, album, hlmask);
  pm->playlist_set_selection(playlist, bit_array_true(), hlmask);
  pm->set_active_playlist(playlist);
  pm->playlist_execute_default_action(playlist, first);

}

void EngineWindow::cmdHighlightPlaylistContent() {
  auto targetAlbum = engineThread->sendSync<EM::GetTargetAlbum>().get();
  if (targetAlbum) {
    t_size playlist =
        playlist_manager::get()->find_playlist(configData->InSourePlaylistGetName());
    bit_array_bittable hlmask;
    t_size first = GetHighlightMask(playlist, targetAlbum.value(), hlmask);
    //todo: display flickering on playlist browser
    //playlist_manager::get()->playlist_set_selection(playlist, bit_array_true(), bit_array_false());
    playlist_manager::get()->playlist_set_selection(playlist, bit_array_true(), hlmask);
    playlist_manager::get()->playlist_ensure_visible(playlist, first);
    //playlist_manager::get()->playlist_set_focus_item(playlist, first);
  }
}

bool EngineWindow::cmdActivateVisualization(int ndx) {
  CoverConfigMap coverconfigs = configData->CoverConfigs;
  int session_pos = configData->sessionCompiledCPInfo.get().first;
  if (ndx <= coverconfigs.size()) {
    CoverConfig new_cc;
    // -1 reset to default
    if (ndx == -1) {
      // restore saved cover config from settings
      ndx = configData->GetCCPosition();
      if (ndx != session_pos) {
        auto& cfg_cc = coverconfigs.at(configData->CoverConfigSel.c_str());
        new_cc = cfg_cc;
      } else
        return false;
    } else {
      // 0 default, 1..9 get 0 to 8 cover configs
      if (ndx != session_pos) {
        auto& [ndxname, cfg_cc] = *std::next(coverconfigs.begin(), ndx);
        new_cc = cfg_cc;
      } else
        return false;
    }

    try {
      auto cfg_ptr = make_shared<CompiledCPInfo>(compileCPScript(new_cc.script.c_str()));
      EngineThread::forEach([ndx, cfg_ptr](EngineThread& t) {
        t.send<EM::ChangeCoverPositionsMessage>(cfg_ptr);
      });
      configData->sessionCompiledCPInfo.set(ndx, cfg_ptr);
    } catch (std::exception& e) {
      // deliver exception
    }
    return true;
  }
  else
    // index out of bounds
    return false;
}
void EngineWindow::cmdToggleLibraryCoverFollowsSelection() {
  configData->CoverFollowsLibrarySelection = !configData->CoverFollowsLibrarySelection;
  if (!configData->CoverFollowsLibrarySelection == true) {
    //..
  }
  else {
    //..
  }
}
void EngineWindow::cmdToggleLibraryFilterSelectorSource(bool locked) {
  configData->SourceActivePlaylist = false;
  configData->SourcePlaylist = false;
  if (!locked) {
    configData->SourceLibrarySelector = !configData->SourceLibrarySelector;
    configData->SourceLibrarySelectorLock &= configData->SourceLibrarySelector;
    if (!configData->SourceLibrarySelector) {
      //shared pointer release on DbReloadWorker destructor
    }

    if (!configData->SourceLibrarySelector && !configData->SourceLibrarySelectorLock)
      engineThread->send<EM::ReloadCollection>();

    if (configData->SourceLibrarySelector && !configData->CoverFollowsLibrarySelection)
      //need the follow selection
      cmdToggleLibraryCoverFollowsSelection();

  }
  else {
    configData->SourceLibrarySelectorLock = !configData->SourceLibrarySelectorLock;
    /*if (!configData->SourceLibrarySelectorLock == true) {
      engineThread->send<EM::ReloadCollection>();
    }*/
  }
}

bool HasImageExtension(pfc::string8 extension) {
  const std::vector<pfc::string8> vext {".jpg", ".jpeg", ".png", ".gif", ".bmp", ".tiff"};
  return std::find(vext.begin(), vext.end(), extension) != vext.end();
}
void EngineWindow::cmdShowAlbumOnExternalViewer(AlbumInfo album) {
  const metadb_handle_list& tracks = album.tracks;
  const metadb_handle_ptr& track = tracks.get_item(0).get_ptr();
  static_api_ptr_t<album_art_manager_v2> aam;
  abort_callback_impl abort;
  try {
    auto extractor =
        aam->open(pfc::list_single_ref_t(track),
                  pfc::list_single_ref_t(configData->GetGuiArt(configData->CenterArt)),
                  abort);
    try {
      album_art_path_list::ptr qp = extractor->query_paths(
          configData->GetGuiArt(configData->CustomCoverFrontArt), abort);

      // int cdebug = qp->get_count();
      pfc::string8 strPicturePath = qp->get_path(0);

      if (!HasImageExtension(pfc::string8(PathFindExtensionA(strPicturePath))))
        return;

      if (configData->CoverUseLegacyExternalViewer) {
        //PhotoViewer
        //from environment
        char* strProgFiles;
        strProgFiles = getenv("ProgramFiles");
        pfc::stringcvt::string_wide_from_utf8_fast bufferWide;
        //command program files photoviewer
        bufferWide.convert(strProgFiles);
        std::wstring wsCmd = (L"\"");
        wsCmd.append(bufferWide);
        wsCmd.append("\\Windows Photo Viewer\\PhotoViewer.dll\", "
            L"ImageView_Fullscreen ");
        //picture path
        bufferWide.convert(strPicturePath);
        wsCmd.append(bufferWide);
        std::wstring wsSystemRoot = (L"\"");
        //system root (c:\windows)
        char* strSysRoot;
        strSysRoot = getenv("SystemRoot");
        std::wstring wsCmdSysRoot;
        pfc::stringcvt::string_wide_from_utf8_fast bufferWideSysRoot;
        bufferWideSysRoot.convert(strSysRoot);
        wsCmdSysRoot.append(bufferWideSysRoot);
        wsCmdSysRoot.append(L"\\system32\\rundll32.exe");

        //command
        ShellExecuteW(this->hWnd, L"open", wsCmdSysRoot.c_str(),
                      wsCmd.c_str(), 0, SW_NORMAL);
      } else {
        //Photos or default
        uShellExecute(0, "open", strPicturePath, 0, 0, SW_NORMAL);
      }
    } catch (const exception_aborted&) {
      return;
    }
  } catch (const exception_album_art_not_found&) {
    return;
  }
}

void EngineWindow::onDamage() {
  engineThread->send<EM::RedrawMessage>();
}

void EngineWindow::setInnerSelection(metadb_handle_list selection, GUID selection_type,
                       bool fromLibrary) {
  if (fromLibrary) {
    this->library_selection = selection;
    this->library_selection_type = selection_type;
  } else {
    this->playlist_selection = selection;
    this->playlist_selection_type = selection_type;
  }
};
void EngineWindow::setSelection(metadb_handle_list selection, bool owner) {
  GUID selguid;
  if (owner) {
    selguid = configData->IsWholeLibrary()
                  ? contextmenu_item::caller_media_library_viewer
                  : contextmenu_item::caller_active_playlist_selection;
  } else {
    selguid = ui_selection_manager::get()->get_selection_type();
  }

  //this->selection = selection;
  setInnerSelection(selection, selguid, configData->IsWholeLibrary());
  //holder selection
  if (owner && (selection.get_count() && selectionHolder.is_valid()))
    selectionHolder->set_selection_ex(selection, selguid);
}

void EngineWindow::onWindowSize(int width, int height) {
  engineThread->send<EM::WindowResizeMessage>(width, height);
}

void EngineWindow::onScroll(double /*xoffset*/, double yoffset) {
  scrollAggregator -= yoffset;
  int m = int(scrollAggregator);
  scrollAggregator -= m;
  engineThread->send<EM::MoveTargetMessage>(m, false);
}

void EngineWindow::onContextMenu(const int x, const int y) {
  POINT pt{};
  std::optional<AlbumInfo> target;

  if (x == -1) {  // Message generated by keyboard
    pt.x = 0;
    pt.y = 0;
    ClientToScreen(hWnd, &pt);
  } else {
    pt.x = x;
    pt.y = y;
    POINT clientPt = pt;
    ScreenToClient(hWnd, &clientPt);
    auto future = engineThread->sendSync<EM::GetAlbumAtCoords>(clientPt.x, clientPt.y);
    target = future.get();
  }
  if (!target) {
    target = engineThread->sendSync<EM::GetTargetAlbum>().get();
  }

  service_ptr_t<contextmenu_manager> cmm;
  contextmenu_manager::g_create(cmm);
  HMENU hMenu = CreatePopupMenu();

  if (target) {
    if (!configData->CtxHideExtViewerMenu) {
      uAppendMenu(hMenu, MF_STRING, ID_OPENEXTERNALVIEWER,
                  "Open External Viewer");
    }
    if (configData->IsWholeLibrary()) {
      if (!configData->CtxHideActionsMenu) {
        if ((configData->EnterKey.length() > 0) &&
            (strcmp(configData->EnterKey, configData->DoubleClick) != 0)) {
          uAppendMenu(hMenu, MF_STRING, ID_ENTER,
                      PFC_string_formatter() << configData->EnterKey << "\tEnter");
        }
        if (configData->DoubleClick.length() > 0) {
          uAppendMenu(
              hMenu, MF_STRING, ID_DOUBLECLICK,
              PFC_string_formatter() << configData->DoubleClick << "\tDouble Click");
        }
        if ((configData->MiddleClick.length() > 0) &&
            (strcmp(configData->MiddleClick, configData->DoubleClick) != 0) &&
            (strcmp(configData->MiddleClick, configData->EnterKey) != 0)) {
          uAppendMenu(
              hMenu, MF_STRING, ID_MIDDLECLICK,
              PFC_string_formatter() << configData->MiddleClick << "\tMiddle Click");
        }
      }
    }
    cmm->init_context(target->tracks, contextmenu_manager::FLAG_SHOW_SHORTCUTS);
    if (cmm->get_root() != nullptr) {
      if (GetMenuItemCount(hMenu) > 0)
        uAppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
      cmm->win32_build_menu(hMenu, ID_CONTEXT_FIRST, ID_CONTEXT_LAST);
      uAppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    }
  }

  uAppendMenu(hMenu, MF_STRING, ID_PREFERENCES,
              PFC_string_formatter() << component_NAME << " Preferences...");

  if (!configData->CtxHideDisplayMenu)
    AppendDisplayContextMenuOptions(&hMenu);
  if (!configData->CtxHideSelectorMenu)
    AppendSelectorContextMenuOptions(&hMenu, false);
  else {
    //temporal fix while selector functionality is discussed for inclusion or not in future releases
    //show only the library selection toggle if appropiate
    bool bonlylibseltoggle = true;
    AppendSelectorContextMenuOptions(&hMenu, bonlylibseltoggle);
  }
  if (!configData->CtxHidePlaylistMenu)
    AppendPlaylistContextMenuOptions(&hMenu);

  menu_helpers::win32_auto_mnemonics(hMenu);

  const int cmd = TrackPopupMenu(
      hMenu, TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NOANIMATION, pt.x, pt.y,
      0, hWnd, nullptr);
  DestroyMenu(hMenu);
  if ((cmd == ID_LIBRARY_COVER_FOLLOWS_SELECTION) ||
      (cmd == ID_LIBRARY_FILTER_SELECTOR_AS_SOURCE) ||
      (cmd == ID_LIBRARY_FILTER_SELECTOR_LOCK)) {
    OnSelectorContextCommand(&hMenu, cmd, this);
  } else if ((cmd == ID_PLAYLIST_FOLLOWS_PL_SELECTION) ||
             (cmd == ID_PLAYLIST_CURRENT_AS_SOURCE) ||
             (cmd == ID_PLAYLIST_SOURCE_SET) ||
             (cmd == ID_PLAYLIST_ACTIVE_AS_SOURCE)) {
    OnPlaylistContextCommand(&hMenu, cmd, this);
  } else if (cmd >= ID_DISPLAY_0 && cmd <= ID_DISPLAY_9) {
    OnDisplayContextCommand(&hMenu, cmd, this);
  } else if (cmd == ID_PREFERENCES) {
    static_api_ptr_t<ui_control>()->show_preferences(guid_config_dialog);
  } else if (cmd == ID_OPENEXTERNALVIEWER) {
    cmdShowAlbumOnExternalViewer(target.value());
  } else if (cmd == ID_ENTER) {
    executeAction(configData->EnterKey, target.value(), this->hWnd,
                  ActionGetBlockFlag(configData->CustomActionFlag, AB_ENTER));
  } else if (cmd == ID_DOUBLECLICK) {
    executeAction(configData->DoubleClick, target.value(), this->hWnd,
                  ActionGetBlockFlag(configData->CustomActionFlag, AB_DOUBLECLICK));
  } else if (cmd == ID_MIDDLECLICK) {
    executeAction(configData->MiddleClick, target.value(), this->hWnd,
                  ActionGetBlockFlag(configData->CustomActionFlag, AB_MIDDLECLICK));
  } else if (cmd >= ID_CONTEXT_FIRST && cmd <= ID_CONTEXT_LAST) {
    cmm->execute_by_id(cmd - ID_CONTEXT_FIRST);
  }
}

void external_selection_callback::on_selection_changed(metadb_handle_list_cref p_selection) {

  if (engineWindow.getSelection(configData->IsWholeLibrary()) == p_selection) {

    if (!configData->IsWholeLibrary() && configData->CoverHighLightPlaylistSelection) {
      //highlight
      engineWindow.cmdHighlightPlaylistContent();
      //console log...
    }

    //
    // Engine::setTarget(...) from EngineWindow::OnFocus or EM messages:
    // MoveTargetMessage, MoveToCurrentTrack, MoveToAlbumMessage
    // finish here

    return;
  }

  GUID gui_sel_anon = contextmenu_item::caller_undefined;
  GUID gui_sel_type = ui_selection_manager::get()->get_selection_type();

  //Library viewers (Library Viewer, Coverflow in Library mode or ESPlaylist in Library mode, ...)
  bool srcFromLibraryViewer = gui_sel_type == contextmenu_item::caller_media_library_viewer;
  //Active Playlist selection (Coverflow playlist mode, ESPlaylist active playlist mode, ...)
  bool srcFromActivePlaylistSelection = gui_sel_type == contextmenu_item::caller_active_playlist_selection;
  //Undefined, ej. Spider monkey ListTree or ESPlaylist in Selected playlist mode
  bool srcFromUndefined = gui_sel_type == contextmenu_item::caller_undefined;


  //debug
  //bool srcFromNowPlaying = gui_sel_type == contextmenu_item::caller_now_playing;
  //if (srcFromNowPlaying)
  //  srcFromNowPlaying = srcFromNowPlaying;
  //if (srcFromUndefined) {
  //  // console log...
  //}
  //

  //not at function top to ease type debugging
  if (p_selection.get_count() == 0)
    return;  // do nothing

  if (gui_sel_type == contextmenu_item::caller_media_library_viewer ||
      (gui_sel_type == gui_sel_anon && configData->CoverFollowsAnonymSelection)) {
    if (configData->CoverFollowsLibrarySelection && configData->IsWholeLibrary()) {
      if (!engineWindow.engineThread.has_value())
        return;

      pfc::string8_fast_aggressive keyBuffer;
      titleformat_object::ptr keyBuilder;
      titleformat_compiler::get()->compile_safe_ex(keyBuilder, configData->Group);
      p_selection.get_item(0)->format_title(nullptr, keyBuffer, keyBuilder, nullptr);

      auto selection_albuminfo =
          engineWindow.engineThread->sendSync<EM::GetTrackAlbum>(p_selection.get_item(0))
              .get();
      bool bselection_exist = selection_albuminfo.has_value();

      auto target_albuminfo =
          engineWindow.engineThread->sendSync<EM::GetTargetAlbum>().get();

      pfc::string8_fast_aggressive sortBuffer;
      pfc::stringcvt::string_wide_from_utf8_fast sortBufferWide;
      titleformat_object::ptr sortBuilder;
      titleformat_compiler::get()->compile_safe_ex(sortBuilder, configData->Sort);

      shared_ptr<metadb_handle_list> shared_selection =
          make_unique<metadb_handle_list>(p_selection);

      //(A)    selection on external Library Viewer with Selector enabled
      //       ReloadCollectionFromList loads while component remains unfocused
      //       Selection will be refreshed when focus is restored
      //       Holder is not refreshed letting selection ownership unmodified
      //
      //(B)    Selection on external Library Viewer without Selector or Selector Locked

      if (configData->CoverFollowsLibrarySelection) {
        if (configData->SourceLibrarySelector && (!configData->SourceLibrarySelectorLock)) {
          //(A)
          engineWindow.setSelection(p_selection, false);
          engineWindow.engineThread->send<EM::ReloadCollectionFromList>(shared_selection);
        } else {
          //(B)
          if (stricmp_utf8(target_albuminfo.value().pos.key.c_str(), keyBuffer) != 0)
            engineWindow.engineThread->send<EM::MoveToCurrentTrack>( p_selection.get_item(0));
        }
      }
    }
  } else if (gui_sel_type == contextmenu_item::caller_active_playlist_selection) {
    // this type of selection is processed by class PlaylistCallback
  }
}

/*
Debug
AlbumInfo aaalbuminfo = AlbumInfo{
    dbiter.value()->title,
    keyBuffer.c_str(),
    sortBufferWide
};
albuminfo = e.db.getAlbumInfo(dbiter.value());
*/


}  // namespace engine
