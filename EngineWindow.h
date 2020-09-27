#pragma once
// clang-format off
#include "ContainerWindow.fwd.h"
#include "DbAlbumInfo.h"
#include "EngineThread.fwd.h"
// clang-format on
#include "configdata.h"
#include "myactions.h"
#include "style_manager.h"
#include "utils.h"

namespace engine {

enum {
  ID_ENTER = 1,
  ID_DOUBLECLICK,
  ID_MIDDLECLICK,
  ID_OPENEXTERNALVIEWER,
  ID_PREFERENCES,
  ID_PLAYLIST_FOLLOWS_PL_SELECTION,
  ID_PLAYLIST_CURRENT_AS_SOURCE,
  ID_PLAYLIST_SOURCE_SET,
  ID_PLAYLIST_ACTIVE_AS_SOURCE,
  ID_LIBRARY_COVER_FOLLOWS_SELECTION,
  ID_LIBRARY_FILTER_SELECTOR_AS_SOURCE,
  ID_LIBRARY_FILTER_SELECTOR_LOCK,
  ID_SUBMENU_SELECTOR,
  ID_SUBMENU_PLAYLIST,
  ID_DISPLAY_0,
  ID_DISPLAY_1,
  ID_DISPLAY_2,
  ID_DISPLAY_3,
  ID_DISPLAY_4,
  ID_DISPLAY_5,
  ID_DISPLAY_6,
  ID_DISPLAY_7,
  ID_DISPLAY_8,
  ID_DISPLAY_9,
  ID_SUBMENU_DISPLAY,
  ID_CONTEXT_FIRST,
  ID_CONTEXT_LAST = ID_CONTEXT_FIRST + 1000,
};

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
  EngineWindow(ContainerWindow& container, StyleManager& styleManager,
               ui_element_instance_callback_ptr defaultUiCallback);

  void setWindowSize(int width, int height);
  void makeContextCurrent();
  void swapBuffers();
  void onDamage();
  void setSelection(metadb_handle_list selection);

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
  metadb_handle_list selection;
  ui_selection_holder::ptr selectionHolder;

  GLFWContext glfwContext;
  unique_ptr<GLFWwindow,
             std::integral_constant<decltype(&glfwDestroyWindow), glfwDestroyWindow>>
      glfwWindow;

 public:
  HWND hWnd = nullptr;
  std::optional<EngineThread> engineThread;
};
}  // namespace engine
