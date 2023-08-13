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
  ID_COVER_UI_SETTINGS_LABEL,
  ID_COVER_FOLLOWS_PLAY_NOW,
  ID_COVER_SETS_SELECTION,
  ID_LIBRARY_COVER_FOLLOWS_SELECTION,
  ID_PLAYLIST_SET_PL_SELECTION,
  ID_PLAYLIST_FOLLOWS_PL_SELECTION,
  ID_PLAYLIST_CURRENT_AS_SOURCE,
  ID_PLAYLIST_SOURCE_SET,
  ID_PLAYLIST_ACTIVE_AS_SOURCE,
  ID_PLAYLIST_GROUPED,
  ID_PLAYLIST_HILIGHT,
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
  ID_SUBMENU_LIBRARY_SELECTIONS,
  ID_CONTEXT_FIRST_DISPLAY,
  ID_CONTEXT_LAST_DISPLAY = ID_CONTEXT_FIRST_DISPLAY + 1000,
};

class GLFWContext {
 public:
  GLFWContext();
  NO_MOVE_NO_COPY(GLFWContext);
  ~GLFWContext();

 private:
  static int count;
};
class external_selection_callback
    : public ui_selection_callback_impl_base_ex<
          playlist_callback::flag_on_items_selection_change |
          playlist_callback::flag_on_playlist_activate> {

 public:
  external_selection_callback(EngineWindow& ew) : engineWindow(ew){};
  void on_selection_changed(metadb_handle_list_cref p_selection);
  void reregister(unsigned int flags) {
      auto api = ui_selection_manager_v2::get();
      api->unregister_callback(this);
      api->register_callback(this, flags);
  }

 private:
  EngineWindow& engineWindow;
};

using ::db::AlbumInfo;

class EngineWindow {
 public:
  EngineWindow(ContainerWindow& container, render::StyleManager& styleManager,
               ui_element_instance_callback_ptr defaultUiCallback)
      : defaultUiCallback(defaultUiCallback), container(container),
        externalSelectionCallback(*this) {
    TRACK_CALL_TEXT("EngineWindow::EngineWindow");
    createWindow();
    engineThread.emplace(*this, styleManager);
    glfwShowWindow(glfwWindow.get());

    // externalSelectionCallback.ui_selection_callback_activate(true);
  }
  // bool query_capability(const GUID& cap) override{}
  void setWindowSize(int width, int height);
  void makeContextCurrent();
  void swapBuffers();
  void onDamage();
  void setSelection(metadb_handle_list selection, bool owner);
  const metadb_handle_list_ref getSelection(bool fromLibrary) {
    return fromLibrary? library_selection : playlist_selection;
  }
  LRESULT on_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) { /*int kk = 0*/; }
  void cmdTogglePlaylistGrouped();
  void cmdTogglePlaylistHiLight();
  void cmdToggleActivePlaylistSource();
  void cmdTogglePlaylistSource();
  void cmdAssignPlaylistSource();
  void cmdPlaylistSourcePlay(const AlbumInfo& album);
  void cmdHighlightPlaylistContent();
  bool cmdActivateVisualization(LPARAM lpActVis, LPARAM hwnd);
  void cmdToggleLibraryFilterSelectorSource(bool lock);
  void cmdToggleLibraryCoverFollowsSelection();
  void cmdShowAlbumOnExternalViewer(AlbumInfo album);

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

  void setInnerSelection(metadb_handle_list selection, GUID selection_type,
                         bool fromLibrary);

 public:
  ContainerWindow& container;

 private:
  double scrollAggregator = 0;
  const ui_element_instance_callback_ptr defaultUiCallback;

  metadb_handle_list library_selection;
  GUID library_selection_type;
  metadb_handle_list playlist_selection;
  GUID playlist_selection_type;
  ui_selection_holder::ptr selectionHolder;
  external_selection_callback externalSelectionCallback;

  GLFWContext glfwContext;
  unique_ptr<GLFWwindow,
             std::integral_constant<decltype(&glfwDestroyWindow), glfwDestroyWindow>>
      glfwWindow;

 public:
  HWND hWnd = nullptr;
  std::optional<EngineThread> engineThread;
};
}  // namespace engine
