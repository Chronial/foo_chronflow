#include "stdafx.h"
#include <set>
#include "base.h"
#include "config.h"

#include "AppInstance.h"
#include "Console.h"
#include "DbAlbumCollection.h"
#include "MyActions.h"
#include "PlaybackTracer.h"
#include "RenderThread.h"
#include "ScriptedCoverPositions.h"
#include "TrackDropSource.h"
#include "TimerOwner.h"

#define MAINWINDOW_CLASSNAME L"foo_chronflow MainWindow"

#define MINIMIZE_CHECK_TIMEOUT 10000 // milliseconds

class FindAsYouType : TimerOwner {
	static const int typeTimeout = 1000; // milliseconds
	pfc::string8 enteredString;
	AppInstance* appInstance;
	bool playbackTracerLocked;
public:
	FindAsYouType(AppInstance* instance)
		: TimerOwner(instance), appInstance(instance){
		clearSearch();
		playbackTracerLocked = false;
	}
	bool onChar(WPARAM wParam){
		collection_read_lock lock(appInstance);
		switch (wParam)
		{
			case 1: // any other nonchar character
			case 0x09: // Process a tab. 
				break;

			case 0x08: // Process a backspace. 
				removeChar();
				break;


			case 0x0A: // Process a linefeed. 
			case 0x0D: // Process a carriage return. 
			case 0x1B: // Process an escape. 
				clear();
				break;

			default: // Process any writeable character
				enterChar(wParam);
				break;
		}
		return 0;

	}
	void enterChar(wchar_t c){
		pfc::string8 newString(enteredString);
		newString << pfc::stringcvt::string_utf8_from_wide(&c, 1);
		if (doSearch(newString)){
			enteredString = newString;
		} else {
			MessageBeep(-1);
		}
		lockPlaybackTracer();
		setTimer(typeTimeout);
	}
	void removeChar(){
		enteredString.truncate(enteredString.length() - 1);
		if (enteredString.length() == 0){
			unlockPlaybackTracer();
		} else {
			doSearch(enteredString);
			lockPlaybackTracer();
			setTimer(typeTimeout);
		}
	}
	void clear(){
		unlockPlaybackTracer();
		killTimer();
		clearSearch();
	}
	void timerProc(){
		clear();
	}

private:
	void lockPlaybackTracer(){
		if (!playbackTracerLocked){
			playbackTracerLocked = true;
			appInstance->playbackTracer->lock();
		}
	}
	void unlockPlaybackTracer(){
		if (playbackTracerLocked){
			playbackTracerLocked = false;
			appInstance->playbackTracer->unlock();
		}
	}
	bool doSearch(const char* searchFor){
		//console::print(pfc::string_formatter() << "searching for: " << searchFor);
		ASSERT_SHARED(appInstance->albumCollection);
		CollectionPos pos;
		if (appInstance->albumCollection->performFayt(searchFor, pos)){
			appInstance->albumCollection->setTargetPos(pos);
			return true;
		} else {
			return false;
		}
	}
	void clearSearch(){
		enteredString.reset();
	}
};


class RenderWindow {
	AppInstance* appInstance;
	GLFWwindow* window;
	HWND hWnd;
	double scrollAggregator = 0;
	std::unique_ptr<FindAsYouType> findAsYouType;
public:

	RenderWindow(AppInstance* appInstance) : appInstance(appInstance){
		findAsYouType = make_unique<FindAsYouType>(appInstance);

		// TODO: handle window creation errors?
		glfwWindowHint(GLFW_DECORATED, false);
		// TODO: respect cfgMultisampling
		glfwWindowHint(GLFW_SAMPLES, 8);
		glfwWindowHint(GLFW_FOCUSED, false);
		glfwWindowHint(GLFW_RESIZABLE, false);
		window = glfwCreateWindow(640, 480, "foo_chronflow render window", NULL, NULL);
		appInstance->glfwWindow = window;

		hWnd = glfwGetWin32Window(window);
		SetParent(hWnd, appInstance->mainWindow);
		LONG nNewStyle = GetWindowLong(hWnd, GWL_STYLE) & ~WS_POPUP | WS_CHILDWINDOW;
		SetWindowLong(hWnd, GWL_STYLE, nNewStyle);
		SetWindowSubclass(hWnd, &RenderWindow::WndProc, 0, reinterpret_cast<DWORD_PTR>(this));


		glfwWindowHint(GLFW_VISIBLE, false);
		appInstance->glfwLoaderWindow = glfwCreateWindow(10, 10, "foo_chronflow texloader window", NULL, appInstance->glfwWindow);
		HWND glfwLoaderHwnd = glfwGetWin32Window(appInstance->glfwLoaderWindow);
		SetParent(glfwLoaderHwnd, appInstance->mainWindow);


		glfwSetWindowUserPointer(window, this);
		glfwSetScrollCallback(window, &RenderWindow::onScroll);
		glfwSetWindowRefreshCallback(window, &RenderWindow::onDamage);
		glfwSetWindowSizeCallback(window, &RenderWindow::onWindowSize);
	};
	~RenderWindow(){
		glfwDestroyWindow(appInstance->glfwWindow);
		appInstance->glfwWindow = nullptr;
		glfwDestroyWindow(appInstance->glfwLoaderWindow);
		appInstance->glfwLoaderWindow = nullptr;
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
			// TODO / FIXME get this information
			//if (callback->is_edit_mode_enabled()){
			//	return DefWindowProc(hWnd, uMsg, wParam, lParam);
			/*} else*/ {
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
		} else if (wParam == VK_RIGHT || wParam == VK_LEFT || wParam == VK_NEXT || wParam == VK_PRIOR
			|| wParam == VK_HOME || wParam == VK_END) {
			int move = 0;
			if (wParam == VK_RIGHT){
				move = +1;
			} else if (wParam == VK_LEFT){
				move = -1;
			} else if (wParam == VK_NEXT){
				move = +10;
			} else if (wParam == VK_PRIOR){
				move = -10;
			} else if (wParam == VK_END){
				// TODO: This is a dirty hack
				move = +10000000;
			} else if (wParam == VK_HOME){
				move -= 10000000;
			}
			if (move){
				collection_read_lock lock(appInstance);
				move *= LOWORD(lParam);
				appInstance->albumCollection->moveTargetBy(move);
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
				if(ksm->on_keydown_auto_context(tracks, wParam, contextmenu_item::caller_media_library_viewer))
					return true;
			} else {
				if(ksm->on_keydown_auto(wParam))
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


class Chronflow : public ui_element_instance {
public:
	static GUID g_get_guid() {
		// {1D56881C-CA24-470c-944A-DED830F9E95D}
		static const GUID guid_foo_chronflow = { 0x1d56881c, 0xca24, 0x470c, { 0x94, 0x4a, 0xde, 0xd8, 0x30, 0xf9, 0xe9, 0x5d } };
		return guid_foo_chronflow;
	}
	static GUID g_get_subclass() { return ui_element_subclass_media_library_viewers; }

	GUID get_guid() { return Chronflow::g_get_guid(); }
	GUID get_subclass() { return Chronflow::g_get_subclass(); }

	static void g_get_name(pfc::string_base & out) { out = "Chronflow"; }
	static const char * g_get_description() { return "Displays a 3D rendering of the Album Art in your Media Library"; }

	static ui_element_config::ptr g_get_default_configuration() { return ui_element_config::g_create_empty(g_get_guid()); }

	HWND get_wnd() {
		return appInstance->mainWindow;
	};

private:
	ULONG_PTR gdiplusToken;

	bool mainWinMinimized = true;
	ui_element_config::ptr config;
	const ui_element_instance_callback_ptr callback;
public:
	AppInstance* appInstance;
	static std::set<Chronflow*> instances;

public:
	Chronflow(HWND parent, ui_element_config::ptr config, ui_element_instance_callback_ptr p_callback)
			: callback(p_callback), config(config) {
		appInstance = new AppInstance();
		IF_DEBUG(Console::create());

		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		// TODO: catch errors from this call
		Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);


		appInstance->mainWindow = createWindow(parent);
		appInstance->albumCollection = make_unique<DbAlbumCollection>(appInstance);

		appInstance->renderWindow = make_unique<RenderWindow>(appInstance);
		appInstance->renderer = make_unique<RenderThread>(appInstance);
		auto rendererInitMsg = make_shared<RTInitDoneMessage>();
		appInstance->renderer->send(rendererInitMsg);
		rendererInitMsg->getAnswer();

		{
			// Do we need this lock?
			collection_read_lock lock(appInstance);
			appInstance->playbackTracer = make_unique<PlaybackTracer>(appInstance);
		}
		appInstance->startCollectionReload();
		instances.insert(this);
	}
	~Chronflow(){
		IF_DEBUG(Console::println(L"Destroying UiElement"));
		if (appInstance)
			DestroyWindow(appInstance->mainWindow);
	}

	// Called on window destruction
	void shutdown(){
		instances.erase(this);
		appInstance->reloadWorker.synchronize()->reset();
		appInstance->playbackTracer.reset();
		appInstance->renderer.reset();
		appInstance->renderWindow.reset();
		appInstance->albumCollection.reset();
		appInstance->mainWindow = nullptr;
		delete pfc::replace_null_t(appInstance);

		Gdiplus::GdiplusShutdown(gdiplusToken);
	}

	void set_configuration(ui_element_config::ptr config) { this->config = config; }
	ui_element_config::ptr get_configuration() { return config; }

	static bool registerWindowClass(){
		HINSTANCE myInstance = core_api::get_my_instance();

		WNDCLASS wc = { 0 };
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS | CS_NOCLOSE;
		wc.lpfnWndProc = (WNDPROC)Chronflow::WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = sizeof(Chronflow *);
		wc.hInstance = myInstance;
		wc.hIcon = LoadIcon(NULL, IDI_HAND);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = NULL;
		wc.lpszMenuName = NULL;
		wc.lpszClassName = MAINWINDOW_CLASSNAME;

		return RegisterClass(&wc) != 0;
	}

private:
	LRESULT MessageHandler (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		if (!appInstance) // We are already shut down – do we need this? set a breakpoint here and check
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		switch (uMsg){
			case WM_DESTROY:
				shutdown();
				return 0;
			case WM_SIZE:
			{
				if (appInstance->glfwWindow){
					glfwSetWindowSize(appInstance->glfwWindow, LOWORD(lParam), HIWORD(lParam));
				}
				return 0;
			}
			case WM_DISPLAYCHANGE:
			case WM_DEVMODECHANGE:
			{
				if (appInstance->renderer)
					appInstance->renderer->send(make_shared<RTDeviceModeMessage>());
				return 0;
			}
			case WM_TIMER:
				switch (wParam){
					case IDT_CHECK_MINIMIZED:
						onCheckMinimizeTimerHit();
						break;
				}
				return 0;
			case WM_ERASEBKGND:
				return TRUE;
			case WM_PAINT:
			{
				if (GetUpdateRect(hWnd, 0, FALSE)){
					appInstance->renderWindow->onDamage();
					if (mainWinMinimized){
						mainWinMinimized = false;
						appInstance->renderer->send(make_shared<RTWindowShowMessage>());
					}
					SetTimer(hWnd, IDT_CHECK_MINIMIZED, MINIMIZE_CHECK_TIMEOUT, 0);
				}
			}
		}
		return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}
	static LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		Chronflow* chronflow = 0;
		if (uMsg == WM_NCCREATE){
			chronflow = reinterpret_cast<Chronflow*>(((CREATESTRUCT*)lParam)->lpCreateParams);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)chronflow);
		} else {
			chronflow = reinterpret_cast<Chronflow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		}
		if (chronflow == 0)
			return DefWindowProc(hWnd,uMsg,wParam,lParam);
		return chronflow->MessageHandler(hWnd, uMsg, wParam, lParam);
	}
	void onCheckMinimizeTimerHit(){
		if (appInstance->isMainWinMinimized()){
			if (!mainWinMinimized){
				mainWinMinimized = true;
				appInstance->renderer->send(make_shared<RTWindowHideMessage>());
				KillTimer(appInstance->mainWindow, IDT_CHECK_MINIMIZED);
			}
		}
	}

	HWND createWindow(HWND parent){
		HWND		hWnd;

		WIN32_OP(hWnd = CreateWindowEx(
			0,									// Extended Style For The Window
			MAINWINDOW_CLASSNAME,				// Class Name
			L"ChronFlow MainWin",				// Window Title
			WS_CHILD |							// Defined Window Style
			WS_CLIPSIBLINGS |					// Required Window Style
			WS_CLIPCHILDREN,					// Required Window Style*/
			CW_USEDEFAULT, CW_USEDEFAULT,		// Window Position
			CW_USEDEFAULT, CW_USEDEFAULT,		// Window Dimensions
			parent,								// No Parent Window
			NULL,								// No Menu
			core_api::get_my_instance(),		// Instance
			(void*)this));

		return hWnd;
	}
};

std::set<Chronflow*> Chronflow::instances = std::set<Chronflow*>();


class UiElement : public ui_element {
public:
	GUID get_guid() { return Chronflow::g_get_guid(); }
	GUID get_subclass() { return Chronflow::g_get_subclass(); }
	void get_name(pfc::string_base & out) { Chronflow::g_get_name(out); }
	ui_element_instance::ptr instantiate(HWND parent, ui_element_config::ptr cfg, ui_element_instance_callback::ptr callback) {
		PFC_ASSERT(cfg->get_guid() == get_guid());
		service_nnptr_t<Chronflow> item = new service_impl_t<Chronflow>(parent, cfg, callback);
		return item;
	}
	ui_element_config::ptr get_default_configuration() { return Chronflow::g_get_default_configuration(); }
	ui_element_children_enumerator_ptr enumerate_children(ui_element_config::ptr cfg) { return NULL; }
	bool get_description(pfc::string_base & out) { out = Chronflow::g_get_description(); return true; }
};

static service_factory_single_t<UiElement> uiElement;

void getAppInstances(pfc::array_t<AppInstance*> &instances){
	instances.set_count(0);
	for (Chronflow* i : Chronflow::instances){
		instances.append_single_val(i->appInstance);
	}
}


class InitHandler : public init_stage_callback {
public:
	void on_init_stage(t_uint32 stage){
		if (stage == init_stages::after_library_init){
			initGlfw();
		} else if (stage == init_stages::before_ui_init){
			registerWindowClasses();
		}
	}

private:
	void initGlfw(){
		glfwSetErrorCallback([](int error, const char* description){
			pfc::string8 msg = "foo_chronflow glfw error: ";
			msg.add_string(description);
			console::print(msg);
		});
		if (!glfwInit()){
			console::print("foo_chronflow failed to initialize glfw.");
			// TODO: Handle this somehow
		}
	}
	void registerWindowClasses(){
		// Note: We do not need to unregister these classes as it happens automatically when foobar quits
		if (!Chronflow::registerWindowClass()){
			errorPopupWin32("Failed to register MainWindow class");
		}
	}
};

static service_factory_single_t<InitHandler> initHandler;

class QuitHandler : public initquit {
public:
	void on_quit(){
		glfwTerminate();
	}
};

static service_factory_single_t<QuitHandler> quitHandler;