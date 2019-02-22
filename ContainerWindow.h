#pragma once
#include "EngineWindow.h"

struct GdiContext {
  ULONG_PTR token{};
  GdiContext() {
    Gdiplus::GdiplusStartupInput input;
    Gdiplus::GdiplusStartup(&token, &input, nullptr);
  }
  GdiContext(const GdiContext&) = delete;
  GdiContext& operator=(const GdiContext&) = delete;
  GdiContext(const GdiContext&&) = delete;
  GdiContext& operator=(GdiContext&&) = delete;
  ~GdiContext() { Gdiplus::GdiplusShutdown(token); }
};

class ContainerWindow {
 public:
  static bool registerWindowClass();

  explicit ContainerWindow(HWND parent,
                           ui_element_instance_callback_ptr duiCallback = nullptr);
  NO_MOVE_NO_COPY(ContainerWindow);
  ~ContainerWindow();

  HWND hwnd = nullptr;

  void destroyEngineWindow(std::string errorMessage);

 private:
  HWND createWindow(HWND parent);
  GdiContext gdiContext;
  bool mainWinMinimized = true;

  std::unique_ptr<EngineWindow> engineWindow;
  std::string engineError{};

  void drawErrorMessage();
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  LRESULT MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
