#pragma once
#include "EngineThread.h"
#include "utils.h"

struct AlbumInfo;
class ContainerWindow;

class GLFWContext {
 public:
  GLFWContext();
  NO_MOVE_NO_COPY(GLFWContext);
  ~GLFWContext();

 private:
  static int count;
};

class EngineWindow {
 public:
  EngineWindow(ContainerWindow& container,
               ui_element_instance_callback_ptr defaultUiCallback);

  void setWindowSize(int width, int height);
  void makeContextCurrent();
  void swapBuffers();
  void onDamage();

 private:
  void createWindow();
  LRESULT messageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam);
  bool onChar(WPARAM wParam);
  void onMouseClick(UINT uMsg, WPARAM wParam, LPARAM lParam);
  void doDragStart(const AlbumInfo& album);
  void onClickOnAlbum(const AlbumInfo& album, UINT uMsg);
  bool onKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam);
  void onWindowSize(int width, int height);
  void onScroll(double xoffset, double yoffset);
  void onContextMenu(int x, int y);

 public:
  ContainerWindow& container;

 private:
  double scrollAggregator = 0;
  ui_element_instance_callback_ptr defaultUiCallback;
  GLFWContext glfwContext;
  unique_ptr_del<GLFWwindow, glfwDestroyWindow> glfwWindow;

 public:
  HWND hWnd = nullptr;
  std::optional<EngineThread> engineThread;
};
