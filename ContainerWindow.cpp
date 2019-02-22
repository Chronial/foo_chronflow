#include "ContainerWindow.h"

#include "Engine.h"
#include "EngineThread.h"
#include "EngineWindow.h"
#include "utils.h"

constexpr int minimizeCheckTimeout = 10'000;  // milliseconds

ContainerWindow::ContainerWindow(HWND parent,
                                 ui_element_instance_callback_ptr duiCallback) {
  TRACK_CALL_TEXT("ContainerWindow::startup");
  IF_DEBUG(console::create());

  hwnd = createWindow(parent);
  try {
    engineWindow = make_unique<EngineWindow>(*this, duiCallback);
  } catch (std::exception& e) {
    engineError = e.what();
  }
}

HWND ContainerWindow::createWindow(HWND parent) {
  constexpr wchar_t* mainwindowClassname = L"foo_chronflow ContainerWindow";

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
                              L"ChronFlow MainWin",  //  Title
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

LRESULT ContainerWindow::MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam,
                                        LPARAM lParam) {
  switch (uMsg) {
    case WM_DESTROY:
      engineWindow.reset();
      break;
    case WM_NCDESTROY:
      this->hwnd = nullptr;
      break;
    case WM_SIZE: {
      if (engineWindow) {
        engineWindow->setWindowSize(LOWORD(lParam), HIWORD(lParam));
      }
      return 0;
    }
    case WM_DISPLAYCHANGE:
    case WM_DEVMODECHANGE: {
      if (engineWindow)
        engineWindow->engineThread->send<EM::DeviceModeMessage>();
      return 0;
    }
    case WM_TIMER:
      switch (wParam) {
        case IDT_CHECK_MINIMIZED:
          if (!mainWinMinimized && !ui_control::get()->is_visible()) {
            mainWinMinimized = true;
            if (engineWindow)
              engineWindow->engineThread->send<EM::WindowHideMessage>();
            KillTimer(hWnd, IDT_CHECK_MINIMIZED);
          }
          break;
      }
      return 0;
    case WM_ERASEBKGND:
      return TRUE;
    case WM_PAINT: {
      if (GetUpdateRect(hWnd, nullptr, FALSE)) {
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
        SetTimer(hWnd, IDT_CHECK_MINIMIZED, minimizeCheckTimeout, nullptr);
      }
    }
  }
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
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
           uT(PFC_string_formatter() << "foo_chronflow error:\n"
                                     << engineError.c_str()),
           -1, &rc, DT_WORDBREAK | DT_CENTER | DT_NOCLIP | DT_NOPREFIX);
  EndPaint(hwnd, &ps);
}

LRESULT CALLBACK ContainerWindow::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                          LPARAM lParam) {
  ContainerWindow* chronflow = nullptr;
  if (uMsg == WM_NCCREATE) {
    chronflow = static_cast<ContainerWindow*>(
        (reinterpret_cast<CREATESTRUCT*>(lParam))->lpCreateParams);
    SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(chronflow));
  } else {
    chronflow = reinterpret_cast<ContainerWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
  }
  if (chronflow == nullptr)
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
  return chronflow->MessageHandler(hWnd, uMsg, wParam, lParam);
}

void ContainerWindow::destroyEngineWindow(std::string errorMessage) {
  if (!engineWindow)
    return;
  engineWindow.reset();
  engineError = errorMessage;
}
