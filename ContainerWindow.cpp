// clang-format off
#include "ContainerWindow.h"
#include "EngineThread.h" //(1)
#include "EngineWindow.h" //(2)
#include "Engine.h"

#include "configData.h"
#include "ConfigCoverConfigs.h"
#include "utils.h"
// clang-format off

namespace engine {

using coverflow::configData;
using coverflow::builtInCoverConfigs;

using EM = engine::Engine::Messages;

constexpr int minimizeCheckTimeout = 10'000;  // milliseconds
constexpr int minimizeCheckTimerId = 665;

ContainerWindow::ContainerWindow(HWND parent, StyleManager& styleManager,
                                 ui_element_instance_callback_ptr duiCallback) {
  TRACK_CALL_TEXT("ContainerWindow::startup");
  createWindow(parent);

  try {
#ifdef _WIN64
    ensureScriptControlVersion();
#endif

    auto cInfo = configData->sessionCompiledCPInfo.get();
    ensureIsSet(cInfo.first, cInfo.second);
    engineWindow = make_unique<EngineWindow>(*this, styleManager, duiCallback);
  } catch (std::exception& e) {
    engineError = e.what();
    FB2K_console_formatter() << AppNameInternal << " failed to initialize:\n" << e.what();
  }
  catch (...) {
     FB2K_console_formatter() << AppNameInternal << " failed to initialize:\n";
  }
}

bool ContainerWindow::ensureScriptControlVersion() {
    bool ver_ok = false;
    WCHAR pszBuff[4096];
    lstrcpy(pszBuff, L"%SystemRoot%\\System32\\tsc64.dll");
    ExpandEnvironmentStrings(pszBuff, pszBuff, 4096);
    ver_ok = isCheckBaseVersion(pszBuff, 1, 2, 5, 4);
    if (!ver_ok) {
      pfc::string8 message("TablacusScriptControl version error.\r\n(minimum required: v.1.2.5.4)");
      FB2K_console_formatter() << AppNameInternal << " encountered an error:\n"
                           << message.c_str();
      MessageBoxW(nullptr,
              uT(PFC_string_formatter() << message
                 << "\r\n\r\nDownload site: Tablacus ScriptControl Github repository."),
              uT(PFC_string_formatter() << "Error in " << AppNameInternal), MB_OK | MB_ICONWARNING);
    }
    ver_ok &= checkScriptControl();
  return ver_ok;
}

void ContainerWindow::ensureIsSet(int listposition, shared_ptr<CompiledCPInfo>& sessionCompiledCPInfo) {
  shared_ptr<CompiledCPInfo> empty_ptr;
  if (sessionCompiledCPInfo != empty_ptr)
    return;

  CompiledCPInfo cInfo;
  try {
    // Try to compile the user's script

    std::string config = configData->CoverConfigs.at(coverConfigScript_Sel.c_str()).script;
    cInfo = compileCPScript(config.c_str());
  } catch (std::exception&) {
    // Fall back to the default script
    cInfo = compileCPScript(builtInCoverConfigs()[configData->CoverConfigSel.c_str()].script.c_str());
  }
  int listpos = coverConfigScript_ndx;
  configData->sessionCompiledCPInfo.set(listpos, make_shared<CompiledCPInfo>(cInfo));
}

HWND ContainerWindow::getEngineWnd() const {
  return engineWindow->hWnd;
}

void ContainerWindow::ApplyCoverConfig(bool bcompile, size_t ndx) {
  pfc::string8 script;
  if (ndx == ~0) {
    script = configData->CoverConfigs.at(configData->CoverConfigSel.c_str()).script.c_str();
  }
  else {
    auto it = configData->CoverConfigs.cbegin();
    std::advance(it, ndx);
    
    script = it->second.script.c_str();
  }

  try {
    int settings_ndx = coverConfigScript_ndx;

    CompiledCPInfo cinfo = compileCPScript(script.c_str());

    auto ccptr = make_shared<CompiledCPInfo>(cinfo);
    configData->sessionCompiledCPInfo.set(settings_ndx, ccptr);
    std::pair<int, shared_ptr<CompiledCPInfo>> cpInfo = { settings_ndx, ccptr };
    EngineThread::forEach(
        [&cpInfo, hwnd = engineWindow->hWnd](EngineThread& t) { t.send<EM::ChangeCoverPositionsMessage>(cpInfo.second, (LPARAM)hwnd); });

  } catch (std::exception&) {

  }
}

HWND ContainerWindow::createWindow(HWND parent) {
  LPCWSTR lpszClassName = L"foo_chronflow_mod ContainerWindow";

  static bool classRegistered = [&] {
    WNDCLASS wc = {0};
    wc.lpszClassName = lpszClassName;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS | CS_NOCLOSE;
    wc.lpfnWndProc = ContainerWindow::WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(ContainerWindow*);
    wc.hInstance = core_api::get_my_instance();
    wc.hIcon = LoadIcon(nullptr, IDI_HAND);  // NOLINT
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);  // NOLINT
    wc.hbrBackground = nullptr;
    wc.lpszMenuName = nullptr;
    check(RegisterClass(&wc));
    return true;
  }();

  CoRegisterClassObject(CLSID_Chron_Control,
    (IClassFactory*)&m_chronClassFactory,
    CLSCTX_LOCAL_SERVER,
    REGCLS_MULTIPLEUSE, &m_chronClassFactoryRegID);

  return check(CreateWindowEx(0,  // Extended Style For The Window
                              lpszClassName,  // Class Name
                              L"foo_chronflow_mod container", //  Title
                              WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,  // Style
                              CW_USEDEFAULT, CW_USEDEFAULT,  // Window Position
                              CW_USEDEFAULT, CW_USEDEFAULT,  // Window Dimensions
                              parent, nullptr, core_api::get_my_instance(),
                              static_cast<void*>(this)));
};

ContainerWindow::~ContainerWindow() {

  HRESULT hres = CoRevokeClassObject(m_chronClassFactoryRegID);
  if (!SUCCEEDED(hres)) {
    FB2K_console_formatter() << AppNameInternal << " failed to revoke IChronControl factory.\n";
  }

  if (hwnd)
    DestroyWindow(hwnd);
}

LRESULT CALLBACK ContainerWindow::WndProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
  ContainerWindow* self = nullptr;
  if (msg == WM_NCCREATE) {
    self = static_cast<ContainerWindow*>(
        (reinterpret_cast<CREATESTRUCT*>(lp))->lpCreateParams);
    SetWindowLongPtr(wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    if (self != nullptr)
      self->hwnd = wnd;
  } else {
    self = reinterpret_cast<ContainerWindow*>(GetWindowLongPtr(wnd, GWLP_USERDATA));
  }
  if (self == nullptr)
    return DefWindowProc(wnd, msg, wp, lp);

  PFC_ASSERT(wnd == self->hwnd);
  if (msg == WM_NCDESTROY) {
    self->hwnd = nullptr;
    SetWindowLongPtr(wnd, GWLP_USERDATA, NULL);
    return DefWindowProc(wnd, msg, wp, lp);
  } else {
    return self->MessageHandler(msg, wp, lp);
  }
}

LRESULT ContainerWindow::MessageHandler(UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
    case WM_DESTROY:
      engineWindow.reset();
      break;
    case WM_SIZE: {
      if (engineWindow)
        engineWindow->setWindowSize(LOWORD(lp), HIWORD(lp));
      return 0;
    }
    case WM_DISPLAYCHANGE:
    case WM_DEVMODECHANGE: {
      if (engineWindow)
        engineWindow->engineThread->send<EM::DeviceModeMessage>();
      return 0;
    }
    case WM_TIMER:
      switch (wp) {
        case minimizeCheckTimerId:
          if (!mainWinMinimized && !ui_control::get()->is_visible()) {
            mainWinMinimized = true;
            if (engineWindow)
              engineWindow->engineThread->send<EM::WindowHideMessage>();
            KillTimer(hwnd, minimizeCheckTimerId);
          }
          return 0;
      }
      break;
    case WM_ERASEBKGND:
      return TRUE;
    case WM_PAINT: {
      if (GetUpdateRect(hwnd, nullptr, FALSE)) {
        SetTimer(hwnd, minimizeCheckTimerId, minimizeCheckTimeout, nullptr);
        if (mainWinMinimized) {
          mainWinMinimized = false;
          if (engineWindow)
            engineWindow->engineThread->send<EM::WindowShowMessage>();
        }
        if (engineWindow) {
          engineWindow->onDamage();
        } else {
          drawErrorMessage();
          return 0;
        }
      }
      break;
    }
  }
  return DefWindowProc(hwnd, msg, wp, lp);
}

void ContainerWindow::drawErrorMessage() {
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(hwnd, &ps);
  RECT rc;
  GetClientRect(hwnd, &rc);
  FillRect(hdc, &rc, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
  if (rc.bottom > 50)
    rc.top += 10;
  if (rc.right - rc.left > 200) {
    rc.left += 10;
    rc.right -= 10;
  }
  SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
  SetTextColor(hdc, RGB(40, 0, 0));
  DrawText(hdc,
           uT(PFC_string_formatter() << AppNameInternal << " error:\n"
                                     << engineError.c_str()),
           -1, &rc, DT_WORDBREAK | DT_CENTER | DT_NOCLIP | DT_NOPREFIX);
  EndPaint(hwnd, &ps);
}

void ContainerWindow::destroyEngineWindow(std::string errorMessage) {
  if (!engineWindow)
    return;

  engineWindow.reset();
  engineError = errorMessage;
  FB2K_console_formatter() << AppNameInternal << " encountered an error:\n"
                           << errorMessage.c_str();
}

//x ui ------------------
//host to dlg

void ContainerWindow::set_uicfg(stream_reader_formatter<>* data, t_size p_size, abort_callback & p_abort) {

  //todo: coverArt
  bool berror = true;
  uint32_t dummy_coverArt = 0;

  uint32_t version, disp_info_index, display_flag, reserved_flag;
  pfc::string8 pl_guid, pl_act_guid, plname, actplname, str_reserved1, str_reserved2;

  version = disp_info_index = display_flag = reserved_flag = 0;

  if (p_size) {
    try {

        *data >> version;
        *data >> disp_info_index;
        *data >> dummy_coverArt;
        *data >> display_flag;
        *data >> reserved_flag;

        *data >> pl_guid;
        *data >> plname;
        *data >> pl_act_guid;
        *data >> actplname;

        *data >> str_reserved1;
        *data >> str_reserved2;

        berror = false;
    }
    catch (exception_io_data_truncation e) {
      FB2K_console_formatter() << "Coverflow failed to parse configuration: " << e.what();
    } catch (exception_io_data e) {
      FB2K_console_formatter() << "Coverflow failed to parse configuration" << e.what();
    }
  }


  if (berror || !p_size)
  {
    ResetCoverDisp();
  }
  else {

    size_t sz_disp_info_ndx = static_cast<size_t>(disp_info_index);

    SetDisplayFlag(display_flag);

    if (sz_disp_info_ndx < coverflow::configData->CoverConfigs.size()
      /*sz_disp_info_ndx != ~0 && sz_disp_info_ndx != window.GetCoverConfigNdx()*/) {
      SetCoverConfigNdx(sz_disp_info_ndx);
      auto it = configData->CoverConfigs.begin();
      std::advance(it, sz_disp_info_ndx);
    
      coverConfigScript_Sel = it->first.c_str();

      ApplyCoverConfig(false, sz_disp_info_ndx);
    }

    SetSourcePlaylistGUID(pl_guid);
    SetSourceActivePlaylistGUID(pl_act_guid);
    SetSourcePlaylistName(plname);
    SetSourceActivePlaylistName(actplname);
  }

}

//dlg to host
void ContainerWindow::get_uicfg(stream_writer_formatter<>* out, abort_callback & p_abort) const {

  *out << 1; //version

  *out << static_cast<uint32_t>(GetConstCoverConfigNdx());  // dispinfo index
  *out << static_cast<uint32_t>(0);                         // todo: coverArt;
  *out << static_cast<uint32_t>(GetDisplayFlag());          // display flag
  *out << static_cast<uint32_t>(0);                         // reserved flag

  *out << GetSourcePlaylistGUID();
  *out << GetSourcePlaylistName();
  *out << GetSourceActivePlaylistGUID();
  *out << GetSourceActivePlaylistName();

  pfc::string8 str_rsv = "reserved";
  *out << str_rsv; //str_reserved1;
  *out << str_rsv; //str_reserved2;
}

} // namespace engine
