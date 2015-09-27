#include "stdafx.h"
#include "base.h"
#include "config.h"

#include "AppInstance.h"
#include "Console.h"
#include "DbAlbumCollection.h"
#include "FindAsYouType.h"
#include "MyActions.h"
#include "PlaybackTracer.h"
#include "RenderThread.h"
#include "TrackDropSource.h"

class RenderWindow {
	AppInstance* appInstance;
	GLFWwindow* window;
	HWND hWnd;
	double scrollAggregator = 0;
	std::unique_ptr<FindAsYouType> findAsYouType;
	ui_element_instance_callback_ptr defaultUiCallback;
public:

	RenderWindow(AppInstance* appInstance, ui_element_instance_callback_ptr defaultUiCallback)
		: appInstance(appInstance), defaultUiCallback(defaultUiCallback){
		TRACK_CALL_TEXT("RenderWindow::RenderWindow");
		findAsYouType = make_unique<FindAsYouType>(appInstance);

		// TODO: handle window creation errors?
		glfwDefaultWindowHints();
		glfwWindowHint(GLFW_DECORATED, false);
		glfwWindowHint(GLFW_SAMPLES, cfgMultisampling ? cfgMultisamplingPasses : 0);
		glfwWindowHint(GLFW_FOCUSED, false);
		glfwWindowHint(GLFW_RESIZABLE, false);
		window = glfwCreateWindow(640, 480, "foo_chronflow render window", NULL, NULL);
		if (!window){
			throw std::runtime_error("Failed to create opengl window");
		}
		appInstance->glfwWindow = window;

		hWnd = glfwGetWin32Window(window);
		SetParent(hWnd, appInstance->mainWindow);
		LONG nNewStyle = GetWindowLong(hWnd, GWL_STYLE) & ~WS_POPUP | WS_CHILDWINDOW;
		SetWindowLong(hWnd, GWL_STYLE, nNewStyle);
		SetWindowSubclass(hWnd, &RenderWindow::WndProc, 0, reinterpret_cast<DWORD_PTR>(this));


		glfwWindowHint(GLFW_VISIBLE, false);
		appInstance->glfwLoaderWindow = glfwCreateWindow(10, 10, "foo_chronflow texloader window", NULL, appInstance->glfwWindow);
		if (!appInstance->glfwLoaderWindow){
			glfwDestroyWindow(window);
			appInstance->glfwWindow = nullptr;
			throw std::runtime_error("Failed to create opengl loader window");
		}
		HWND glfwLoaderHwnd = glfwGetWin32Window(appInstance->glfwLoaderWindow);
		SetParent(glfwLoaderHwnd, appInstance->mainWindow);


		glfwSetWindowUserPointer(window, this);
		glfwSetScrollCallback(window, &RenderWindow::onScroll);
		glfwSetWindowRefreshCallback(window, &RenderWindow::onDamage);
		glfwSetWindowSizeCallback(window, &RenderWindow::onWindowSize);
	};
	~RenderWindow(){
		glfwDestroyWindow(appInstance->glfwLoaderWindow);
		appInstance->glfwLoaderWindow = nullptr;
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
		{
			collection_read_lock l(appInstance);
			auto msg = make_shared<RTGetPosAtCoordsMessage>(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			appInstance->renderer->send(msg);
			auto posPtr = msg->getAnswer();
			if (posPtr){
				onClickOnCover(*posPtr, uMsg);
			}
			return 0;
		}

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
			}
			return 0;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if (onKEYDOWN(uMsg, wParam, lParam))
				return 0;
			break;
		case WM_CHAR:
			if (cfgFindAsYouType){
				findAsYouType->onChar(wParam);
				return 0;
			}
			break;
		}

		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void onClickOnCover(CollectionPos clickedCover, UINT uMsg){
		if (uMsg == WM_MBUTTONDOWN){
			appInstance->albumCollection->setTargetPos(clickedCover);
			appInstance->playbackTracer->userStartedMovement();
			executeAction(cfgMiddleClick, clickedCover);
		} else if (uMsg == WM_LBUTTONDOWN){
			POINT pt;
			GetCursorPos(&pt);
			if (DragDetect(hWnd, pt)) {
				metadb_handle_list tracks;
				appInstance->albumCollection->getTracks(clickedCover, tracks);

				static_api_ptr_t<playlist_incoming_item_filter> piif;
				pfc::com_ptr_t<IDataObject> pDataObject = piif->create_dataobject_ex(tracks);
				pfc::com_ptr_t<IDropSource> pDropSource = TrackDropSource::g_create(hWnd);

				DWORD effect;
				DoDragDrop(pDataObject.get_ptr(), pDropSource.get_ptr(), DROPEFFECT_COPY, &effect);
			} else {
				appInstance->albumCollection->setTargetPos(clickedCover);
				appInstance->playbackTracer->userStartedMovement();
			}
		} else if (uMsg == WM_LBUTTONDBLCLK){
			appInstance->albumCollection->setTargetPos(clickedCover);
			appInstance->playbackTracer->userStartedMovement();
			executeAction(cfgDoubleClick, clickedCover);
		}
	}

	bool onKEYDOWN(UINT uMsg, WPARAM wParam, LPARAM lParam){
		if (wParam == VK_RETURN){
			collection_read_lock lock(appInstance);
			if (appInstance->albumCollection->getCount()){
				executeAction(cfgEnterKey, appInstance->albumCollection->getTargetPos());
				return true;
			}
		} else if (wParam == VK_F5){
			appInstance->startCollectionReload();
			return true;
		} else if (wParam == VK_F6){
			collection_read_lock lock(appInstance);
			appInstance->playbackTracer->moveToNowPlaying();
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
			collection_read_lock lock(appInstance);
			if (appInstance->albumCollection->getCount()){
				appInstance->albumCollection->moveTargetBy(move);
				appInstance->playbackTracer->userStartedMovement();
			}
			return true;
		} else if (wParam == VK_HOME || wParam == VK_END) {
			collection_read_lock lock(appInstance);
			if (appInstance->albumCollection->getCount()){
				CollectionPos newTarget;
				if (wParam == VK_END){
					newTarget = --appInstance->albumCollection->end();
				} else if (wParam == VK_HOME){
					newTarget = appInstance->albumCollection->begin();
				}
				appInstance->albumCollection->setTargetPos(newTarget);
				appInstance->playbackTracer->userStartedMovement();
				return true;
			}
		} else if (!(cfgFindAsYouType && // disable hotkeys that interfere with find-as-you-type
			(uMsg == WM_KEYDOWN) &&
			((wParam > 'A' && wParam < 'Z') || (wParam > '0' && wParam < '9') || (wParam == ' ')) &&
			((GetKeyState(VK_CONTROL) & 0x8000) == 0))){
			collection_read_lock lock(appInstance);
			static_api_ptr_t<keyboard_shortcut_manager> ksm;
			metadb_handle_list tracks;
			if (appInstance->albumCollection->getCount() &&
				appInstance->albumCollection->getTracks(appInstance->albumCollection->getTargetPos(), tracks)){
				if (ksm->on_keydown_auto_context(tracks, wParam, contextmenu_item::caller_media_library_viewer))
					return true;
			} else {
				if (ksm->on_keydown_auto(wParam))
					return true;
			}
		}
		return false;
	}


	static void onDamage(GLFWwindow* window){
		reinterpret_cast<RenderWindow*>(glfwGetWindowUserPointer(window))->onDamage();
	}
	void onDamage(){
		appInstance->renderer->send(make_shared<RTRedrawMessage>());
	}

	static void onWindowSize(GLFWwindow* window, int width, int height){
		reinterpret_cast<RenderWindow*>(glfwGetWindowUserPointer(window))->onWindowSize(width, height);
	}
	void onWindowSize(int width, int height){
		auto msg = make_shared<RTWindowResizeMessage>(width, height);
		appInstance->renderer->send(msg);
	}

	static void onScroll(GLFWwindow* window, double xoffset, double yoffset){
		reinterpret_cast<RenderWindow*>(glfwGetWindowUserPointer(window))->onScroll(xoffset, yoffset);
	}
	void onScroll(double xoffset, double yoffset){
		collection_read_lock l(appInstance);
		scrollAggregator -= yoffset;
		int m = int(scrollAggregator);
		scrollAggregator -= m;
		appInstance->albumCollection->moveTargetBy(m);
		appInstance->playbackTracer->userStartedMovement();
	}

	void onContextMenu(const int x, const int y){
		collection_read_lock lock(appInstance);
		PlaybackTracerScopeLock tracerLock(*(appInstance->playbackTracer));
		POINT pt;
		metadb_handle_list tracks;
		unique_ptr<CollectionPos> target;

		if (x == -1){ // Message generated by keyboard
			pt.x = 0;
			pt.y = 0;
			ClientToScreen(hWnd, &pt);
		} else {
			pt.x = x;
			pt.y = y;
			POINT clientPt = pt;
			ScreenToClient(hWnd, &clientPt);
			auto msg = make_shared<RTGetPosAtCoordsMessage>(pt.x, pt.y);
			appInstance->renderer->send(msg);
			auto clickedCoverPtr = msg->getAnswer();
			if (clickedCoverPtr){
				target = make_unique<CollectionPos>(*clickedCoverPtr);
			}
		}
		if (!target && appInstance->albumCollection->getCount()){
			target = make_unique<CollectionPos>(appInstance->albumCollection->getTargetPos());
		}
		if (target){
			appInstance->albumCollection->getTracks(*target, tracks);
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

			cmm->init_context(tracks, contextmenu_manager::FLAG_SHOW_SHORTCUTS);
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
			executeAction(cfgEnterKey, *target, tracks);
		} else if (cmd == ID_DOUBLECLICK){
			executeAction(cfgDoubleClick, *target, tracks);
		} else if (cmd == ID_MIDDLECLICK){
			executeAction(cfgMiddleClick, *target, tracks);
		} else if (cmd >= ID_CONTEXT_FIRST && cmd <= ID_CONTEXT_LAST){
			cmm->execute_by_id(cmd - ID_CONTEXT_FIRST);
		}
	}


	inline void executeAction(const char * action, CollectionPos forCover){
		metadb_handle_list tracks;
		appInstance->albumCollection->getTracks(forCover, tracks);
		executeAction(action, forCover, tracks);
	}
	void executeAction(const char * action, CollectionPos forCover, const metadb_handle_list& tracks){
		pfc::string8 albumTitle;
		appInstance->albumCollection->getTitle(forCover, albumTitle);
		for (int i = 0; i < tabsize(g_customActions); i++){
			if (stricmp_utf8(action, g_customActions[i]->actionName) == 0){
				g_customActions[i]->run(tracks, albumTitle);
				return;
			}
		}
		GUID commandGuid;
		if (menu_helpers::find_command_by_name(action, commandGuid)){
			menu_helpers::run_command_context(commandGuid, pfc::guid_null, tracks);
		}
	}
};
