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
  GdiContext gdiContext;
  bool mainWinMinimized = true;

 public:
  HWND hwnd = nullptr;

  explicit ContainerWindow(HWND parent,
                           ui_element_instance_callback_ptr duiCallback = nullptr);
  ContainerWindow(const ContainerWindow&) = delete;
  ContainerWindow& operator=(const ContainerWindow&) = delete;
  ContainerWindow(const ContainerWindow&&) = delete;
  ContainerWindow& operator=(ContainerWindow&&) = delete;
  ~ContainerWindow();

  static bool registerWindowClass();

  std::optional<EngineWindow> engineWindow;

 private:
  HWND createWindow(HWND parent);

  static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  LRESULT MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
