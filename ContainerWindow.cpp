// clang-format off
#include "ContainerWindow.h"
#include "EngineThread.h" //(1)
#include "EngineWindow.h" //(2)
#include "Engine.h"

#include "configData.h"
#include "ConfigCoverConfigs.h"
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
    auto cInfo = configData->sessionCompiledCPInfo.get();
    ensureIsSet(cInfo.first, cInfo.second);
    engineWindow = make_unique<EngineWindow>(*this, styleManager, duiCallback);
  } catch (std::exception& e) {
    engineError = e.what();
    FB2K_console_formatter() << AppNameInternal << " failed to initialize:\n" << e.what();
  }
}

void ContainerWindow::ensureIsSet(int listposition, shared_ptr<CompiledCPInfo>& sessionCompiledCPInfo) {
  shared_ptr<CompiledCPInfo> empty_ptr;
  if (sessionCompiledCPInfo != empty_ptr)
    return;

  CompiledCPInfo cInfo;
  try {
    // Try to compile the user's script

    std::string config = configData
                         ->CoverConfigs.at(configData->CoverConfigSel.c_str()).script;
    cInfo = compileCPScript(config.c_str());
  } catch (std::exception&) {
    // Fall back to the default script
    cInfo = compileCPScript(builtInCoverConfigs()[defaultCoverConfig].script.c_str());
  }
  using CoverConfigMap = std::map<std::string, coverflow::CoverConfig, ILessUtf8>;

  int listpos = configData->GetCCPosition();

  configData->sessionCompiledCPInfo.set(listpos, make_shared<CompiledCPInfo>(cInfo));
}

void ContainerWindow::applyCoverConfig(bool bcompile) {
  pfc::string8 script = configData->CoverConfigs.at(configData->CoverConfigSel.c_str()).
                                    script.c_str();

  try {
    int settings_ndx = configData->GetCCPosition();
    auto ccptr = make_shared<CompiledCPInfo>(compileCPScript(script));
    configData->sessionCompiledCPInfo.set(settings_ndx, ccptr);
    std::pair<int, shared_ptr<CompiledCPInfo>> cInfo = { settings_ndx, ccptr };
    EngineThread::forEach(
        [&cInfo](EngineThread& t) { t.send<EM::ChangeCoverPositionsMessage>(cInfo.second); });

  } catch (std::exception& e) {

  }
}

HWND ContainerWindow::createWindow(HWND parent) {
  const wchar_t* mainwindowClassname = uT(
      PFC_string_formatter() << AppNameInternal << " ContainerWindow");

  static bool classRegistered = [&] {
    WNDCLASS wc = {0};
    wc.lpszClassName = mainwindowClassname;
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

  return check(CreateWindowEx(0,  // Extended Style For The Window
                              mainwindowClassname,  // Class Name
                              uT(PFC_string_formatter() << AppNameInternal << " container"), //  Title
                              WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,  // Style
                              CW_USEDEFAULT, CW_USEDEFAULT,  // Window Position
                              CW_USEDEFAULT, CW_USEDEFAULT,  // Window Dimensions
                              parent, nullptr, core_api::get_my_instance(),
                              static_cast<void*>(this)));
};

ContainerWindow::~ContainerWindow() {
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
} // namespace engine
