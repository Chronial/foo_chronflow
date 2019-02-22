#pragma once
#include "DbAlbumCollection.h"
#include "EngineThread.h"
#include "utils.h"

class ContainerWindow;

class EngineWindow {
  unique_ptr_del<GLFWwindow, glfwDestroyWindow> glfwWindow;
  double scrollAggregator = 0;
  ui_element_instance_callback_ptr defaultUiCallback;

 public:
  ContainerWindow& container;
  std::optional<EngineThread> engineThread;
  HWND hWnd = nullptr;

  EngineWindow(ContainerWindow& container,
               ui_element_instance_callback_ptr defaultUiCallback);

  void setWindowSize(int width, int height);
  void makeContextCurrent();
  void swapBuffers();
  void onDamage();

 private:
  LRESULT messageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam);
  bool onChar(WPARAM wParam);
  void onMouseClick(UINT uMsg, WPARAM wParam, LPARAM lParam);
  void doDragStart(AlbumInfo album);
  void onClickOnAlbum(AlbumInfo album, UINT uMsg);
  bool onKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam);
  void onWindowSize(int width, int height);
  void onScroll(double xoffset, double yoffset);
  void onContextMenu(int x, int y);
};
