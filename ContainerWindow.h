#pragma once
#include "EngineWindow.h"
#include "style_manager.h"

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
  explicit ContainerWindow(HWND parent, StyleManager& styleManager,
                           ui_element_instance_callback_ptr duiCallback = nullptr);
  NO_MOVE_NO_COPY(ContainerWindow);
  ~ContainerWindow();
  void destroyEngineWindow(std::string errorMessage);
  HWND getHWND() const { return hwnd; };

 private:
  HWND createWindow(HWND parent);
  static LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);
  LRESULT MessageHandler(UINT msg, WPARAM wp, LPARAM lp);
  void drawErrorMessage();

  GdiContext gdiContext;
  bool mainWinMinimized = true;
  std::string engineError{};
  // Note that the HWND might be destroyed by its parent via DestroyWindow()
  // before this class is destroyed.
  HWND hwnd = nullptr;
  std::unique_ptr<EngineWindow> engineWindow;
};
