#include "EngineWindow.h"

#include "lib/win32_helpers.h"

#include "ContainerWindow.h"
#include "Engine.h"
#include "MyActions.h"
#include "PlaybackTracer.h"
#include "TrackDropSource.h"
#include "config.h"
#include "utils.h"

int GLFWContext::count = 0;

GLFWContext::GLFWContext() {
  if (count == 0) {
    glfwSetErrorCallback([](int error, const char* description) {
      throw std::runtime_error(PFC_string_formatter() << "glfw error: " << description);
    });
    if (!glfwInit()) {
      throw std::runtime_error("Failed to initialize glfw");
    }
  }
  ++count;
}

GLFWContext::~GLFWContext() {
  --count;
  if (count == 0) {
    TRACK_CALL(glfwTerminate);
    glfwTerminate();
  }
}

EngineWindow::EngineWindow(ContainerWindow& container, StyleManager& styleManager,
                           ui_element_instance_callback_ptr defaultUiCallback)
    : defaultUiCallback(std::move(defaultUiCallback)), container(container) {
  TRACK_CALL_TEXT("EngineWindow::EngineWindow");

  createWindow();
  engineThread.emplace(*this, styleManager);
  glfwShowWindow(glfwWindow.get());
}

void EngineWindow::createWindow() {
  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_DECORATED, FALSE);
  glfwWindowHint(GLFW_SAMPLES, cfgMultisampling ? cfgMultisamplingPasses : 0);
  glfwWindowHint(GLFW_FOCUSED, FALSE);
  glfwWindowHint(GLFW_RESIZABLE, FALSE);
  glfwWindowHint(GLFW_VISIBLE, FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

  glfwWindow.reset(
      glfwCreateWindow(640, 480, "foo_chronflow render window", nullptr, nullptr));
  if (!glfwWindow) {
    throw std::runtime_error("Unknown error while creating opengl window");
  }
  hWnd = glfwGetWin32Window(glfwWindow.get());

  WIN32_OP_D(SetParent(hWnd, container.getHWND()));
  const LONG nNewStyle = (GetWindowLong(hWnd, GWL_STYLE) & ~WS_POPUP) | WS_CHILDWINDOW;
  SetWindowLong(hWnd, GWL_STYLE, nNewStyle);
  const ULONG_PTR cNewStyle = GetClassLongPtr(hWnd, GCL_STYLE) | CS_DBLCLKS;
  WIN32_OP_D(SetClassLongPtr(hWnd, GCL_STYLE, cNewStyle));
  WIN32_OP_D(SetWindowSubclass(
      hWnd,
      WINLAMBDA([](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR,
                   DWORD_PTR dwRefData) noexcept {
        try {
          return reinterpret_cast<EngineWindow*>(dwRefData)->messageHandler(
              uMsg, wParam, lParam);
        } catch (std::exception& e) {
          FB2K_console_formatter()
              << "Exception in foo_chronflow EngineWindow MessageHandler: " << e.what();
          return DefSubclassProc(hwnd, uMsg, wParam, lParam);
        }
      }),
      0, reinterpret_cast<DWORD_PTR>(this)));

  static auto wrap = [](auto f) noexcept {
    try {
      f();
    } catch (std::exception& e) {
      FB2K_console_formatter()
          << "Exception in foo_chronflow EngineWindow event handler: " << e.what();
    }
  };
  glfwSetWindowUserPointer(glfwWindow.get(), this);
  glfwSetScrollCallback(
      glfwWindow.get(), [](GLFWwindow* window, double xoffset, double yoffset) {
        wrap([&] {
          static_cast<EngineWindow*>(glfwGetWindowUserPointer(window))
              ->onScroll(xoffset, yoffset);
        });
      });
  glfwSetWindowRefreshCallback(glfwWindow.get(), [](GLFWwindow* window) {
    wrap([&] {
      static_cast<EngineWindow*>(glfwGetWindowUserPointer(window))->onDamage();
    });
  });
  glfwSetWindowSizeCallback(
      glfwWindow.get(), [](GLFWwindow* window, int width, int height) {
        wrap([&] {
          static_cast<EngineWindow*>(glfwGetWindowUserPointer(window))
              ->onWindowSize(width, height);
        });
      });
}

void EngineWindow::setWindowSize(int width, int height) {
  glfwSetWindowSize(glfwWindow.get(), width, height);
}

void EngineWindow::makeContextCurrent() {
  glfwMakeContextCurrent(glfwWindow.get());
}

void EngineWindow::swapBuffers() {
  glfwSwapBuffers(glfwWindow.get());
}

LRESULT EngineWindow::messageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_MOUSEACTIVATE:
      SetFocus(hWnd);
      return MA_ACTIVATE;
    case WM_SETFOCUS: {
      selectionHolder = ui_selection_manager::get()->acquire();
      setSelection(selection);
      break;
    }
    case WM_KILLFOCUS: {
      selectionHolder.release();
      break;
    }
    case WM_GETDLGCODE:
      return DLGC_WANTALLKEYS;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
      onMouseClick(uMsg, wParam, lParam);
      return 0;
    case WM_RBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_NCRBUTTONUP:
      // Generate WM_CONTEXTMENU messages
      return DefWindowProc(hWnd, uMsg, wParam, lParam);
    case WM_CONTEXTMENU:
      if (defaultUiCallback.is_valid() && defaultUiCallback->is_edit_mode_enabled()) {
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
      } else {
        onContextMenu(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
      }
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
      if (onKeyDown(uMsg, wParam, lParam))
        return 0;
      break;
    case WM_CHAR:
      if (onChar(wParam))
        return 0;
      break;
  }

  return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

bool EngineWindow::onChar(WPARAM wParam) {
  if (cfgFindAsYouType) {
    engineThread->send<EM::CharEntered>(wParam);
    return true;
  } else {
    return false;
  }
}

void EngineWindow::onMouseClick(UINT uMsg, WPARAM /*wParam*/, LPARAM lParam) {
  auto future = engineThread->sendSync<EM::GetAlbumAtCoords>(
      GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
  auto clickedAlbum = future.get();
  if (!clickedAlbum)
    return;

  if (uMsg == WM_LBUTTONDOWN) {
    POINT pt;
    GetCursorPos(&pt);
    if (DragDetect(hWnd, pt)) {
      doDragStart(clickedAlbum.value());
      return;
    }
  }
  onClickOnAlbum(clickedAlbum.value(), uMsg);
}

void EngineWindow::doDragStart(const AlbumInfo& album) {
  static_api_ptr_t<playlist_incoming_item_filter> piif;
  pfc::com_ptr_t<IDataObject> pDataObject = piif->create_dataobject_ex(album.tracks);
  pfc::com_ptr_t<IDropSource> pDropSource = TrackDropSource::g_create(hWnd);

  DWORD effect;
  DoDragDrop(pDataObject.get_ptr(), pDropSource.get_ptr(), DROPEFFECT_COPY, &effect);
}

void EngineWindow::onClickOnAlbum(const AlbumInfo& album, UINT uMsg) {
  if (uMsg == WM_LBUTTONDOWN) {
    engineThread->send<EM::MoveToAlbumMessage>(album);
  } else if (uMsg == WM_MBUTTONDOWN) {
    executeAction(cfgMiddleClick, album);
  } else if (uMsg == WM_LBUTTONDBLCLK) {
    executeAction(cfgDoubleClick, album);
  }
}

bool EngineWindow::onKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  if (wParam == VK_RETURN) {
    auto targetAlbum = engineThread->sendSync<EM::GetTargetAlbum>().get();
    if (targetAlbum)
      executeAction(cfgEnterKey, targetAlbum.value());
    return true;
  } else if (wParam == VK_F5) {
    engineThread->send<EM::ReloadCollection>();
    return true;
  } else if (wParam == VK_F6) {
    engineThread->send<EM::MoveToNowPlayingMessage>();
    return true;
  } else if (wParam == VK_RIGHT || wParam == VK_LEFT || wParam == VK_NEXT ||
             wParam == VK_PRIOR) {
    int move = 0;
    if (wParam == VK_RIGHT) {
      move = +1;
    } else if (wParam == VK_LEFT) {
      move = -1;
    } else if (wParam == VK_NEXT) {
      move = +10;
    } else if (wParam == VK_PRIOR) {
      move = -10;
    }
    move *= LOWORD(lParam);
    engineThread->send<EM::MoveTargetMessage>(move, false);
    return true;
  } else if (wParam == VK_HOME) {
    engineThread->send<EM::MoveTargetMessage>(-1, true);
    return true;
  } else if (wParam == VK_END) {
    engineThread->send<EM::MoveTargetMessage>(1, true);
    return true;
  } else if (!(cfgFindAsYouType &&  // disable hotkeys that interfere with
                                    // find-as-you-type
               (uMsg == WM_KEYDOWN) &&
               ((wParam > 'A' && wParam < 'Z') || (wParam > '0' && wParam < '9') ||
                (wParam == ' ')) &&
               ((GetKeyState(VK_CONTROL) & 0x8000) == 0))) {
    auto targetAlbum = engineThread->sendSync<EM::GetTargetAlbum>().get();
    static_api_ptr_t<keyboard_shortcut_manager> ksm;
    if (targetAlbum) {
      return ksm->on_keydown_auto_context(
          targetAlbum->tracks, wParam, contextmenu_item::caller_media_library_viewer);
    } else {
      return ksm->on_keydown_auto(wParam);
    }
  }
  return false;
}

void EngineWindow::onDamage() {
  engineThread->send<EM::RedrawMessage>();
}

void EngineWindow::setSelection(metadb_handle_list selection) {
  this->selection = selection;
  if (selection.get_count() && selectionHolder.is_valid())
    selectionHolder->set_selection_ex(
        selection, contextmenu_item::caller_media_library_viewer);
}

void EngineWindow::onWindowSize(int width, int height) {
  engineThread->send<EM::WindowResizeMessage>(width, height);
}

void EngineWindow::onScroll(double /*xoffset*/, double yoffset) {
  scrollAggregator -= yoffset;
  int m = int(scrollAggregator);
  scrollAggregator -= m;
  engineThread->send<EM::MoveTargetMessage>(m, false);
}

void EngineWindow::onContextMenu(const int x, const int y) {
  POINT pt{};
  std::optional<AlbumInfo> target;

  if (x == -1) {  // Message generated by keyboard
    pt.x = 0;
    pt.y = 0;
    ClientToScreen(hWnd, &pt);
  } else {
    pt.x = x;
    pt.y = y;
    POINT clientPt = pt;
    ScreenToClient(hWnd, &clientPt);
    auto future = engineThread->sendSync<EM::GetAlbumAtCoords>(pt.x, pt.y);
    target = future.get();
  }
  if (!target) {
    target = engineThread->sendSync<EM::GetTargetAlbum>().get();
  }

  enum {
    ID_ENTER = 1,
    ID_ENTER2,
    ID_DOUBLECLICK,
    ID_MIDDLECLICK,
    ID_PREFERENCES,
    ID_CONTEXT_FIRST,
    ID_CONTEXT_LAST = ID_CONTEXT_FIRST + 1000,
  };
  service_ptr_t<contextmenu_manager> cmm;
  contextmenu_manager::g_create(cmm);
  HMENU hMenu = CreatePopupMenu();
  if (target) {
    if ((cfgEnterKey.length() > 0) && (strcmp(cfgEnterKey, cfgDoubleClick) != 0)) {
      uAppendMenu(
          hMenu, MF_STRING, ID_ENTER, PFC_string_formatter() << cfgEnterKey << "\tEnter");
    }
    if (cfgDoubleClick.length() > 0) {
      uAppendMenu(hMenu, MF_STRING, ID_DOUBLECLICK,
                  PFC_string_formatter() << cfgDoubleClick << "\tDouble Click");
    }
    if ((cfgMiddleClick.length() > 0) && (strcmp(cfgMiddleClick, cfgDoubleClick) != 0) &&
        (strcmp(cfgMiddleClick, cfgEnterKey) != 0)) {
      uAppendMenu(hMenu, MF_STRING, ID_MIDDLECLICK,
                  PFC_string_formatter() << cfgMiddleClick << "\tMiddle Click");
    }

    cmm->init_context(target->tracks, contextmenu_manager::FLAG_SHOW_SHORTCUTS);
    if (cmm->get_root() != nullptr) {
      if (GetMenuItemCount(hMenu) > 0)
        uAppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
      cmm->win32_build_menu(hMenu, ID_CONTEXT_FIRST, ID_CONTEXT_LAST);
      uAppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    }
  }
  uAppendMenu(hMenu, MF_STRING, ID_PREFERENCES, "Chronflow Preferences...");

  menu_helpers::win32_auto_mnemonics(hMenu);
  const int cmd = TrackPopupMenu(
      hMenu, TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NOANIMATION, pt.x, pt.y,
      0, hWnd, nullptr);
  DestroyMenu(hMenu);
  if (cmd == ID_PREFERENCES) {
    static_api_ptr_t<ui_control>()->show_preferences(guid_configWindow);
  } else if (cmd == ID_ENTER) {
    executeAction(cfgEnterKey, target.value());
  } else if (cmd == ID_DOUBLECLICK) {
    executeAction(cfgDoubleClick, target.value());
  } else if (cmd == ID_MIDDLECLICK) {
    executeAction(cfgMiddleClick, target.value());
  } else if (cmd >= ID_CONTEXT_FIRST && cmd <= ID_CONTEXT_LAST) {
    cmm->execute_by_id(cmd - ID_CONTEXT_FIRST);
  }
}
