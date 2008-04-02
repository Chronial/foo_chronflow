#include "chronflow.h"
#include "config.h"

#define VERSION "0.2.1debug"

DECLARE_COMPONENT_VERSION( "Coverflow pannel", VERSION, 
   "Renders Album Art in a 3d environment\n"
   "By Christian Fersch\n"
   __DATE__ " - " __TIME__ );

extern cfg_int sessionSelectedCover;



class Chronflow : public ui_extension::window {
private:
	ui_extension::window_host_ptr currentHost;

public:
	const bool get_is_single_instance() const {
		return true;
	}
	
	const GUID & get_extension_guid() const {
		// {1D56881C-CA24-470c-944A-DED830F9E95D}
		static const GUID guid_foo_chronflow = { 0x1d56881c, 0xca24, 0x470c, { 0x94, 0x4a, 0xde, 0xd8, 0x30, 0xf9, 0xe9, 0x5d } };
		return guid_foo_chronflow;
	}

	void get_name(pfc::string_base & out) const {
		out = "Chronflow";
	};

	void get_category(pfc::string_base & out) const {
		out = "Panels";
	};

	unsigned get_type() const {
		return ui_extension::type_panel;
	};

	bool is_available(const ui_extension::window_host_ptr & p_host) const {
		if ((currentHost != 0) && p_host->get_host_guid() == currentHost->get_host_guid())
			return false;
		else
			return true;
	};

	HWND create_or_transfer_window(HWND wnd_parent, const ui_extension::window_host_ptr & p_host, const ui_helpers::window_position_t & p_position = ui_helpers::window_position_null) {
		if (isShown){
			ShowWindow(appInstance->mainWindow, SW_HIDE);
			SetParent(appInstance->mainWindow, wnd_parent);
			SetWindowPos(appInstance->mainWindow, NULL, p_position.x, p_position.y, p_position.cx, p_position.cy, SWP_NOZORDER);
			currentHost->relinquish_ownership(appInstance->mainWindow);
		} else {
			if (show(wnd_parent)){
				SetWindowPos(appInstance->mainWindow, NULL, p_position.x, p_position.y, p_position.cx, p_position.cy, SWP_NOZORDER);
				ShowWindow(appInstance->mainWindow, SW_HIDE);
			} else {
				return 0;
			}
		}
		currentHost = p_host;
		isShown = true;
		return appInstance->mainWindow;
	};

	void destroy_window() {
		if (isShown){
			hide();
			currentHost = 0;
			isShown = false;
		}
	};

	HWND get_wnd() const {
		if (appInstance)
			return appInstance->mainWindow;
		else
			return 0;
	};

private:
	bool isShown; // part of the single-istance implementation
	ULONG_PTR gdiplusToken;
	MouseFlicker* mouseFlicker;
	SwapBufferTimer* swapBufferTimer;

	AppInstance* appInstance;
public:
	Chronflow(){
		swapBufferTimer = 0;
		currentHost = 0;
		isShown = false;
		mouseFlicker = 0;
	}
	void init(){
	}
	void quit(){
	}

	bool show(HWND parent){
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

#ifdef _DEBUG
		Console::create();
#endif
		appInstance = new AppInstance();
		try {
			appInstance->coverPos = new ScriptedCoverPositions();
		} catch (...) {
			hide();
			return false;
		}
		if (!registerWindowClass()){
			hide();
			return false;
		}
		appInstance->mainWindow = createWindow(parent);
		if (appInstance->mainWindow == NULL){
			hide();
			return false;
		}
		appInstance->renderer = new Renderer(appInstance);
		if (!appInstance->renderer->attachGlWindow()){
			hide();
			return false;
		}
		if (appInstance->renderer->initMultisampling()){
			DestroyWindow(appInstance->mainWindow);
			appInstance->mainWindow = createWindow(parent);
			if (appInstance->mainWindow == NULL){
				hide();
				return false;
			}
			if (!appInstance->renderer->attachGlWindow()){
				hide();
				return false;
			}
		}
		appInstance->renderer->initGlState();
		swapBufferTimer = new SwapBufferTimer(appInstance, appInstance->mainWindow, &appInstance->lockedRC);
		appInstance->albumCollection = new DbAlbumCollection(appInstance);
		appInstance->texLoader = new AsynchTexLoader(appInstance);
		appInstance->displayPos = new DisplayPosition(appInstance, CollectionPos(appInstance->albumCollection,sessionSelectedCover));
		appInstance->textDisplay = new TextDisplay(appInstance);
		appInstance->playbackTracer = new PlaybackTracer(appInstance);
		mouseFlicker = new MouseFlicker(appInstance);
		
		appInstance->albumCollection->reloadAsynchStart(true);
		return true;
	}
	void hide(){
		delete pfc::replace_null_t(swapBufferTimer); // important to do this here - otherwise it may trigger an swap during deinitalization
		delete pfc::replace_null_t(mouseFlicker);
		if (appInstance->texLoader)
			appInstance->texLoader->stopLoading();
		if (appInstance->mainWindow)
			DestroyWindow(appInstance->mainWindow);
		appInstance->mainWindow = 0;
		UnregisterClass(L"Chronflow",core_api::get_my_instance());
		delete pfc::replace_null_t(appInstance->texLoader);
		delete pfc::replace_null_t(appInstance->renderer);
		delete pfc::replace_null_t(appInstance->displayPos);
		delete pfc::replace_null_t(appInstance->albumCollection);
		delete pfc::replace_null_t(appInstance->coverPos);
		delete pfc::replace_null_t(appInstance->playbackTracer);
		delete pfc::replace_null_t(appInstance->textDisplay);

		delete pfc::replace_null_t(appInstance);

		if (ImgTexture::instanceCount != 0){
			MessageBoxA(0, pfc::string8("ImgTexture Leak: ") << ImgTexture::instanceCount, "foo_chronflow Error", MB_ICONERROR);
		}

		Gdiplus::GdiplusShutdown(gdiplusToken);
	}

private:
	LRESULT MessageHandler (HWND	hWnd,			// Handle For This Window
							UINT	uMsg,			// Message For This Window
							WPARAM	wParam,			// Additional Message Information
							LPARAM	lParam){
		switch (uMsg){
			case WM_COLLECTION_REFRESHED:
				appInstance->albumCollection->reloadAsynchFinish(lParam);
				return 0;
			case WM_DESTROY:
				if (appInstance->renderer){
					delete pfc::replace_null_t(swapBufferTimer);
					appInstance->renderer->destroyGlWindow();
				}
				return 0;
			case WM_NCDESTROY:
				appInstance->mainWindow = 0;
				return 0;
			case WM_SIZE:
				if (appInstance->renderer){
					appInstance->renderer->resizeGlScene(LOWORD(lParam),HIWORD(lParam));
				}
				return 0;
			case WM_DEVMODECHANGE:
			{
				if (swapBufferTimer)
					swapBufferTimer->reloadRefreshRate();
				return 0;
			}
			case WM_MOUSEWHEEL:
			{
				static int zDelta = 0;
				zDelta -= GET_WHEEL_DELTA_WPARAM(wParam);
				int m = zDelta / WHEEL_DELTA;
				zDelta = zDelta % WHEEL_DELTA;
				appInstance->displayPos->setTarget(appInstance->displayPos->getTarget() + m);
				appInstance->playbackTracer->userStartedMovement();
				return 0;
			}
			case WM_MBUTTONDOWN:
			{
				CollectionPos newTarget = appInstance->displayPos->getCenteredPos();
				if (appInstance->renderer->positionOnPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), newTarget)){
					appInstance->displayPos->setTarget(newTarget);
					appInstance->playbackTracer->userStartedMovement();
					executeAction(cfgMiddleClick, newTarget);
				}
				return 0;
			}
			case WM_LBUTTONDBLCLK:
			case WM_LBUTTONDOWN:
			{
				SetFocus(hWnd);
				CollectionPos newTarget = appInstance->displayPos->getCenteredPos();
				if (appInstance->renderer->positionOnPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), newTarget)){
					appInstance->displayPos->setTarget(newTarget);
					appInstance->playbackTracer->userStartedMovement();
					if (uMsg == WM_LBUTTONDBLCLK)
						executeAction(cfgDoubleClick, newTarget);
				} else {
					mouseFlicker->mouseDown(hWnd, GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
				}
				return 0;
			}
			case WM_LBUTTONUP:
				mouseFlicker->mouseUp(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
				return 0;
			case WM_CONTEXTMENU:
				onContextMenu(hWnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				return 0;
			case WM_MOUSEMOVE:
				mouseFlicker->mouseMove(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
				return 0;
			case WM_CAPTURECHANGED:
				mouseFlicker->lostCapture(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
				return 0;
			case WM_TIMER:
				if (wParam == IDT_PLAYBACK_TRACER)
					appInstance->playbackTracer->timerHit();
				return 0;
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
			{
				if (wParam == VK_RETURN){
					executeAction(cfgEnterKey, appInstance->displayPos->getTarget());
					return 0;
				} else if (wParam == VK_F5){
					bool hardRefresh = (GetKeyState(VK_SHIFT) & 0x8000) > 0;
					appInstance->albumCollection->reloadAsynchStart(hardRefresh);
					return 0;
				} else if (wParam == VK_F6){
					appInstance->playbackTracer->moveToNowPlaying();
					return 0;
				} else if (wParam == VK_RIGHT || wParam == VK_LEFT || wParam == VK_NEXT || wParam == VK_PRIOR) {
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
					if (move){
						move *= LOWORD(lParam);
						appInstance->displayPos->setTarget(appInstance->displayPos->getTarget() + move);
						appInstance->playbackTracer->userStartedMovement();
						return 0;
					}
				} else {
					static_api_ptr_t<keyboard_shortcut_manager> ksm;
					if (ksm->on_keydown(ksm->TYPE_MAIN,wParam)){
						return 0;
					} else {
						metadb_handle_list tracks;
						if (appInstance->albumCollection->getTracks(appInstance->displayPos->getTarget(), tracks)){
							if (ksm->on_keydown_context(tracks, wParam, contextmenu_item::caller_undefined))
								return 0;
						}
					}
				}
				break;
			}
			case WM_PAINT:
			{
				ScopeRCLock scopeLock(appInstance->lockedRC);
				double frameStart = Helpers::getHighresTimer();

				appInstance->texLoader->blockUpload();
				appInstance->texLoader->runGlDelete();
				appInstance->displayPos->update();
				appInstance->texLoader->setQueueCenter(appInstance->displayPos->getTarget());
				appInstance->renderer->drawFrame();
				/*double curTime = Helpers::getHighresTimer();
				int sleepFor = int((1000.0/(refreshRate*1.10)) - (1000*(curTime - lastTime))); //1.05 are tolerance
				if (sleepFor > 0)
					SleepEx(sleepFor,false);*/
				
				double frameEnd = Helpers::getHighresTimer();
				appInstance->fpsCounter.recordFrame(frameStart, frameEnd);


				if (!appInstance->displayPos->isMoving()){
					appInstance->texLoader->allowUpload();
					ValidateRect(hWnd,NULL);
				}
				//swapBufferTimer->queueSwap(multipleFrames);
				appInstance->lockedRC.swapBuffers();
				return 0;
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
	inline void executeAction(const char * action, CollectionPos forCover){
		metadb_handle_list tracks;
		appInstance->albumCollection->getTracks(forCover, tracks);
		executeAction(action, forCover, tracks);
	}
	void executeAction(const char * action, CollectionPos forCover, const metadb_handle_list& tracks){
		pfc::string8 albumTitle;
		appInstance->albumCollection->getTitle(forCover, albumTitle);
		for (int i=0; i < tabsize(g_customActions); i++){
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
	void onContextMenu (HWND hWnd, const int x, const int y){
		PlaybackTracerScopeLock lock(appInstance->playbackTracer);
		POINT pt;
		metadb_handle_list tracks;
		CollectionPos target = appInstance->displayPos->getTarget();
		if (x == -1){
			pt.x = 0;
			pt.y = 0;
			ClientToScreen(hWnd, &pt);
		} else {
			pt.x = x;
			pt.y = y;
			POINT clientPt = pt;
			ScreenToClient(hWnd, &clientPt);
			CollectionPos clickedOn = target;
			if(appInstance->renderer->positionOnPoint(clientPt.x, clientPt.y, clickedOn)){
				target = clickedOn;
			}
		}
		appInstance->albumCollection->getTracks(target, tracks);

		enum {
			ID_ENTER = 1,
			ID_ENTER2,
			ID_DOUBLECLICK,
			ID_MIDDLECLICK,
			ID_PREFERENCES,
			ID_CONTEXT_FIRST,
			ID_CONTEXT_LAST = ID_CONTEXT_FIRST + 1000,
		};

		HMENU hMenu = CreatePopupMenu();
		if ((cfgEnterKey.length() > 0) && (strcmp(cfgEnterKey, cfgDoubleClick) != 0))
			uAppendMenu(hMenu, MF_STRING, ID_ENTER, pfc::string8(cfgEnterKey) << "\tEnter");
		if (cfgDoubleClick.length() > 0)
			uAppendMenu(hMenu, MF_STRING, ID_DOUBLECLICK, pfc::string8(cfgDoubleClick) << "\tDouble Click");
		if ((cfgMiddleClick.length() > 0) && (strcmp(cfgMiddleClick, cfgDoubleClick) != 0) && (strcmp(cfgMiddleClick, cfgEnterKey) != 0))
			uAppendMenu(hMenu, MF_STRING, ID_MIDDLECLICK, pfc::string8(cfgMiddleClick) << "\tMiddle Click");

		service_ptr_t<contextmenu_manager> cmm;
		contextmenu_manager::g_create(cmm);
		cmm->init_context(tracks, contextmenu_manager::FLAG_SHOW_SHORTCUTS);
		if (cmm->get_root()){
			if (GetMenuItemCount(hMenu) > 0)
				uAppendMenu(hMenu, MF_SEPARATOR, 0, 0);
			cmm->win32_build_menu(hMenu, ID_CONTEXT_FIRST, ID_CONTEXT_LAST);
			uAppendMenu(hMenu, MF_SEPARATOR, 0, 0);
		}
		uAppendMenu(hMenu, MF_STRING, ID_PREFERENCES, "Chronflow Preferences...");

		menu_helpers::win32_auto_mnemonics(hMenu);
		int cmd = TrackPopupMenu(hMenu, TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NOANIMATION, 
			pt.x, pt.y, 0, hWnd, 0);
		DestroyMenu(hMenu);
		if (cmd == ID_PREFERENCES){
			static_api_ptr_t<ui_control>()->show_preferences(guid_configWindow);
		} else if (cmd == ID_ENTER){
			executeAction(cfgEnterKey, target, tracks);
		} else if (cmd == ID_DOUBLECLICK){
			executeAction(cfgDoubleClick, target, tracks);
		} else if (cmd == ID_MIDDLECLICK){
			executeAction(cfgMiddleClick, target, tracks);
		} else if (cmd >= ID_CONTEXT_FIRST && cmd <= ID_CONTEXT_LAST){
			cmm->execute_by_id(cmd - ID_CONTEXT_FIRST);
		}
		//contextmenu_manager::win32_run_menu_context(hWnd, tracks, &pt);
	}
	bool registerWindowClass(){
		WNDCLASS	wc = {0};						// Windows Class Structure

		wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC| CS_DBLCLKS ;	// Redraw On Size, And Own DC For Window. -- recieve dblclicks (risky)
		wc.lpfnWndProc		= (WNDPROC) this->WndProc;					// WndProc Handles Messages
		wc.cbClsExtra		= 0;									// No Extra Window Data
		wc.cbWndExtra		= 0;									// No Extra Window Data
		wc.hInstance		= core_api::get_my_instance();							// Set The Instance
		wc.hIcon			= LoadIcon(NULL, IDI_HAND);			// Load The Default Icon
		wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
		wc.hbrBackground	= NULL;									// No Background Required For GL
		wc.lpszMenuName		= NULL;									// We Don't Want A Menu
		wc.lpszClassName	= L"Chronflow";							// Set The Class Name
		//wc.cbWndExtra		= sizeof(Chronflow *);


		if (!RegisterClass(&wc))									// Attempt To Register The Window Class
		{
			MessageBox(NULL,L"Failed To Register The Window Class.",L"Foo_chronflow Error",MB_OK|MB_ICONERROR);
			return false;											// Return FALSE
		}
		return true;
	}
	HWND createWindow(HWND parent){
		DWORD		dwExStyle;				// Window Extended Style
		DWORD		dwStyle;				// Window Style
		HWND		hWnd;
		dwExStyle = 0;
		dwStyle = WS_CHILD;

		// Create The Window
		if (!(hWnd=CreateWindowEx(	dwExStyle,							// Extended Style For The Window
									L"Chronflow",						// Class Name
									L"ChronFlow Window",					// Window Title
									dwStyle |							// Defined Window Style
									WS_CLIPSIBLINGS |					// Required Window Style
									WS_CLIPCHILDREN,					// Required Window Style
									CW_USEDEFAULT, CW_USEDEFAULT,		// Window Position
									CW_USEDEFAULT, CW_USEDEFAULT,		// Window Dimensions
									parent,								// No Parent Window
									NULL,								// No Menu
									core_api::get_my_instance(),							// Instance
									(void*)this)))	
		{
			MessageBox(NULL,L"Window Creation Error.",L"Foo_chronflow Error",MB_OK|MB_ICONERROR);
			return 0;								// Return FALSE
		}
		return hWnd;
	}
	friend AppInstance* gGetSingleInstance();
};

static ui_extension::window_factory_single< Chronflow > x_chronflow;

// this links the ConfigWindow to the Single Instance
AppInstance* gGetSingleInstance(){
	return x_chronflow.get_static_instance().appInstance;
}

class MyInitQuit : public initquit
{
   void on_init() 
   {
	   x_chronflow.get_static_instance().init();
   }

   void on_quit() 
   {
	   x_chronflow.get_static_instance().quit();
   }
};
static initquit_factory_t<MyInitQuit> x_myInitQuit;

