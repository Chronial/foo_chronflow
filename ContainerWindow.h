#pragma once
// clang-format off
#include "EngineWindow.fwd.h"
#include "style_manager.h"

#include "cover_positions_compiler.h"
#include "utils.h"
#include "ConfigData.h"

#include "COM_Guid.h"
#include "COM_Module.h"
#include "COM_ClassFactory.h"

// clang-format off

namespace engine {

using render::StyleManager;
using coverflow::configData;

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
  bool ensureScriptControlVersion();
  void ensureIsSet(int listposition, shared_ptr<CompiledCPInfo>& sessionCompiledCPInfo);
  void applyCoverConfig(bool bcompile);

  GdiContext gdiContext;
  bool mainWinMinimized = true;
  std::string engineError{};
  // Note that the HWND might be destroyed by its parent via DestroyWindow()
  // before this class is destroyed.
  HWND hwnd = nullptr;
  std::unique_ptr<EngineWindow> engineWindow;
  ChronClassFactory m_chronClassFactory;
};
} // namespace
