#include "stdafx.h"
#include "base.h"
#include "config.h"

#include "AppInstance.h"
#include "Console.h"
#include "MyActions.h"
#include "PlaybackTracer.h"
#include "RenderThread.h"
#include "TrackDropSource.h"

class RenderWindow {
	AppInstance* appInstance;
	GLFWwindow* window;
	HWND hWnd;
	double scrollAggregator = 0;
	ui_element_instance_callback_ptr defaultUiCallback;
public:

	RenderWindow(AppInstance* appInstance, ui_element_instance_callback_ptr defaultUiCallback)
		: appInstance(appInstance), defaultUiCallback(defaultUiCallback){
		TRACK_CALL_TEXT("RenderWindow::RenderWindow");

		// TODO: handle window creation errors?
		glfwDefaultWindowHints();
		glfwWindowHint(GLFW_DECORATED, false);
		glfwWindowHint(GLFW_SAMPLES, cfgMultisampling ? cfgMultisamplingPasses : 0);
		glfwWindowHint(GLFW_FOCUSED, false);
		glfwWindowHint(GLFW_RESIZABLE, false);
		glfwWindowHint(GLFW_VISIBLE, false);
		window = glfwCreateWindow(640, 480, "foo_chronflow render window", NULL, NULL);
		if (!window){
			throw std::runtime_error("Failed to create opengl window");
		}
		appInstance->glfwWindow = window;

		hWnd = glfwGetWin32Window(window);
		SetParent(hWnd, appInstance->mainWindow);
		LONG nNewStyle = GetWindowLong(hWnd, GWL_STYLE) & ~WS_POPUP | WS_CHILDWINDOW;
		SetWindowLong(hWnd, GWL_STYLE, nNewStyle);
		ULONG_PTR cNewStyle = GetClassLongPtr(hWnd, GCL_STYLE) | CS_DBLCLKS;
		SetClassLongPtr(hWnd, GCL_STYLE, cNewStyle);
		SetWindowSubclass(hWnd, &RenderWindow::WndProc, 0, reinterpret_cast<DWORD_PTR>(this));
		glfwShowWindow(window);

		glfwSetWindowUserPointer(window, this);
		glfwSetScrollCallback(window, &RenderWindow::onScroll);
		glfwSetWindowRefreshCallback(window, &RenderWindow::onDamage);
		glfwSetWindowSizeCallback(window, &RenderWindow::onWindowSize);
	};
	~RenderWindow(){
		glfwDestroyWindow(appInstance->glfwWindow);
		appInstance->glfwWindow = nullptr;
	}

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR  uIdSubclass, DWORD_PTR dwRefData){
		RenderWindow* self = reinterpret_cast<RenderWindow*>(dwRefData);
		return self->messageHandler(uMsg, wParam, lParam);
	}
	LRESULT messageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam){
		switch (uMsg){
		case WM_MOUSEACTIVATE:
			SetFocus(hWnd);
			return MA_ACTIVATE;
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
			if (defaultUiCallback.is_valid() && defaultUiCallback->is_edit_mode_enabled()){
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
	static void onDamage(GLFWwindow* window) {
		reinterpret_cast<RenderWindow*>(glfwGetWindowUserPointer(window))->onDamage();
	}
	static void onWindowSize(GLFWwindow* window, int width, int height) {
		reinterpret_cast<RenderWindow*>(glfwGetWindowUserPointer(window))->onWindowSize(width, height);
	}
	static void onScroll(GLFWwindow* window, double xoffset, double yoffset) {
		reinterpret_cast<RenderWindow*>(glfwGetWindowUserPointer(window))->onScroll(xoffset, yoffset);
	}


	bool onChar(WPARAM wParam) {
		if (cfgFindAsYouType) {
			appInstance->renderer->send<RenderThread::CharEntered>(wParam);
			return true;
		} else {
			return false;
		}
	}

	void onMouseClick(UINT uMsg, WPARAM wParam, LPARAM lParam) {
		auto future = appInstance->renderer->sendSync
			<RenderThread::GetAlbumAtCoords>(
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

	void doDragStart(AlbumInfo album) {
		static_api_ptr_t<playlist_incoming_item_filter> piif;
		pfc::com_ptr_t<IDataObject> pDataObject = piif->create_dataobject_ex(album.tracks);
		pfc::com_ptr_t<IDropSource> pDropSource = TrackDropSource::g_create(hWnd);

		DWORD effect;
		DoDragDrop(pDataObject.get_ptr(), pDropSource.get_ptr(), DROPEFFECT_COPY, &effect);
	}

	void onClickOnAlbum(AlbumInfo album, UINT uMsg){
		if (uMsg == WM_LBUTTONDOWN) {
			appInstance->renderer->send<RenderThread::MoveToAlbumMessage>(album.groupString);
		} else if (uMsg == WM_MBUTTONDOWN){
			executeAction(cfgMiddleClick, album);
		} else if (uMsg == WM_LBUTTONDBLCLK){
			executeAction(cfgDoubleClick, album);
		}
	}

	bool onKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam){
		if (wParam == VK_RETURN){
			auto targetAlbum = appInstance->renderer->sendSync<RenderThread::GetTargetAlbum>().get();
			if (targetAlbum)
				executeAction(cfgEnterKey, targetAlbum.value());
			return true;
		} else if (wParam == VK_F5){
			appInstance->renderer->send<RenderThread::ReloadCollectionMessage>();
			return true;
		} else if (wParam == VK_F6){
			appInstance->renderer->send<RenderThread::MoveToNowPlayingMessage>();
			return true;
		} else if (wParam == VK_RIGHT || wParam == VK_LEFT || wParam == VK_NEXT || wParam == VK_PRIOR){
			int move = 0;
			if (wParam == VK_RIGHT){
				move = +1;
			} else if (wParam == VK_LEFT){
				move = -1;
			} else if (wParam == VK_NEXT){
				move = +10;
			} else if (wParam == VK_PRIOR){
				move = -10;
			}
			move *= LOWORD(lParam);
			appInstance->renderer->send<RenderThread::MoveTargetMessage>(move, false);
			return true;
		} else if (wParam == VK_HOME) {
			appInstance->renderer->send<RenderThread::MoveTargetMessage>(-1, true);
			return true;
		} else if (wParam == VK_END) {
			appInstance->renderer->send<RenderThread::MoveTargetMessage>(1, true);
			return true;
		} else if (!(cfgFindAsYouType && // disable hotkeys that interfere with find-as-you-type
					 (uMsg == WM_KEYDOWN) &&
					 ((wParam > 'A' && wParam < 'Z') || (wParam > '0' && wParam < '9') || (wParam == ' ')) &&
					 ((GetKeyState(VK_CONTROL) & 0x8000) == 0))){
			auto targetAlbum = appInstance->renderer->sendSync<RenderThread::GetTargetAlbum>().get();
			static_api_ptr_t<keyboard_shortcut_manager> ksm;
			if (targetAlbum){
				return ksm->on_keydown_auto_context(targetAlbum->tracks, wParam, contextmenu_item::caller_media_library_viewer);
			} else {
				return ksm->on_keydown_auto(wParam);
			}
		}
		return false;
	}


	void onDamage(){
		appInstance->renderer->send<RenderThread::RedrawMessage>();
	}

	void onWindowSize(int width, int height){
		appInstance->renderer->send<RenderThread::WindowResizeMessage>(
			width, height);
	}

	void onScroll(double xoffset, double yoffset){
		scrollAggregator -= yoffset;
		int m = int(scrollAggregator);
		scrollAggregator -= m;
		appInstance->renderer->send<RenderThread::MoveTargetMessage>(m, false);
	}

	void onContextMenu(const int x, const int y){
		PlaybackTracerScopeLock tracerLock(*(appInstance->playbackTracer));
		POINT pt;
		std::optional<AlbumInfo> target;

		if (x == -1){ // Message generated by keyboard
			pt.x = 0;
			pt.y = 0;
			ClientToScreen(hWnd, &pt);
		} else {
			pt.x = x;
			pt.y = y;
			POINT clientPt = pt;
			ScreenToClient(hWnd, &clientPt);
			auto future = appInstance->renderer->sendSync<RenderThread::GetAlbumAtCoords>(
				pt.x, pt.y);
			target = future.get();
		}
		if (!target){
			target = appInstance->renderer->sendSync<RenderThread::GetTargetAlbum>().get();
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
		if (target){
			if ((cfgEnterKey.length() > 0) && (strcmp(cfgEnterKey, cfgDoubleClick) != 0))
				uAppendMenu(hMenu, MF_STRING, ID_ENTER, pfc::string8(cfgEnterKey) << "\tEnter");
			if (cfgDoubleClick.length() > 0)
				uAppendMenu(hMenu, MF_STRING, ID_DOUBLECLICK, pfc::string8(cfgDoubleClick) << "\tDouble Click");
			if ((cfgMiddleClick.length() > 0) && (strcmp(cfgMiddleClick, cfgDoubleClick) != 0) && (strcmp(cfgMiddleClick, cfgEnterKey) != 0))
				uAppendMenu(hMenu, MF_STRING, ID_MIDDLECLICK, pfc::string8(cfgMiddleClick) << "\tMiddle Click");

			cmm->init_context(target->tracks, contextmenu_manager::FLAG_SHOW_SHORTCUTS);
			if (cmm->get_root()){
				if (GetMenuItemCount(hMenu) > 0)
					uAppendMenu(hMenu, MF_SEPARATOR, 0, 0);
				cmm->win32_build_menu(hMenu, ID_CONTEXT_FIRST, ID_CONTEXT_LAST);
				uAppendMenu(hMenu, MF_SEPARATOR, 0, 0);
			}
		}
		uAppendMenu(hMenu, MF_STRING, ID_PREFERENCES, "Chronflow Preferences...");

		menu_helpers::win32_auto_mnemonics(hMenu);
		int cmd = TrackPopupMenu(hMenu, TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NOANIMATION,
			pt.x, pt.y, 0, hWnd, 0);
		DestroyMenu(hMenu);
		if (cmd == ID_PREFERENCES){
			static_api_ptr_t<ui_control>()->show_preferences(guid_configWindow);
		} else if (cmd == ID_ENTER){
			executeAction(cfgEnterKey, target.value());
		} else if (cmd == ID_DOUBLECLICK){
			executeAction(cfgDoubleClick, target.value());
		} else if (cmd == ID_MIDDLECLICK){
			executeAction(cfgMiddleClick, target.value());
		} else if (cmd >= ID_CONTEXT_FIRST && cmd <= ID_CONTEXT_LAST){
			cmm->execute_by_id(cmd - ID_CONTEXT_FIRST);
		}
	}
};
