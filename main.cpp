#include "chronflow.h"
#include "config.h"

#define VERSION "0.2.0"

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
		return appInstance->mainWindow;
	};

private:
	bool isShown; // part of the single-istance implementation
	int refreshRate;
	ULONG_PTR gdiplusToken;
	UINT multimediaTimerRes;
	MouseFlicker* mouseFlicker;

	AppInstance* appInstance;
public:
	Chronflow(){
		multimediaTimerRes = 0;
		currentHost = 0;
		isShown = false;
	}
	void init(){
		TIMECAPS tc;
		UINT     wTimerRes;
		if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) == TIMERR_NOERROR){
			wTimerRes = min(max(tc.wPeriodMin, 1), tc.wPeriodMax);
		}
		timeBeginPeriod(wTimerRes); 
	}
	void quit(){
		timeEndPeriod(multimediaTimerRes);
		int i = ImgTexture::instanceCount;
		if (ImgTexture::instanceCount != 0){
			MessageBoxA(0, pfc::string8("ImgTexture Leak: ") << ImgTexture::instanceCount, "foo_chronflow Error", MB_ICONERROR);
		}
	}

	static VOID WINAPI GdiPlusDebug (Gdiplus::DebugEventLevel level, CHAR *message){
		console::print(message);
	}
	bool show(HWND parent){
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		gdiplusStartupInput.DebugEventCallback = &GdiPlusDebug;
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
		appInstance->mainWindow = createWindow(parent);
		appInstance->albumCollection = new DbAlbumCollection(appInstance);
		appInstance->texLoader = new AsynchTexLoader(appInstance);
		appInstance->displayPos = new DisplayPosition(appInstance, CollectionPos(appInstance->albumCollection,sessionSelectedCover));
		appInstance->textDisplay = new TextDisplay(appInstance);
		appInstance->renderer = new Renderer(appInstance);
		appInstance->playbackTracer = new PlaybackTracer(appInstance);
		mouseFlicker = new MouseFlicker(appInstance);
		
		if (!appInstance->renderer->setupGlWindow()){
			hide();
			return false;
		}
		appInstance->albumCollection->reloadAsynchStart(true);
		return true;
	}
	void hide(){
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
			IF_DEBUG(MessageBoxA(0, pfc::string8("ImgTexture Leak: ") << ImgTexture::instanceCount, "foo_chronflow Error", MB_ICONERROR));
		}

		Gdiplus::GdiplusShutdown(gdiplusToken);
	}
private:
	LRESULT MessageHandler (HWND	hWnd,			// Handle For This Window
							UINT	uMsg,			// Message For This Window
							WPARAM	wParam,			// Additional Message Information
							LPARAM	lParam){
		if(appInstance->renderer)
			appInstance->renderer->setRenderingContext();

		switch (uMsg){
			case WM_COLLECTION_REFRESHED:
				appInstance->albumCollection->reloadAsynchFinish(lParam);
				return 0;
			case WM_DESTROY:
				if (appInstance->renderer)
					appInstance->renderer->destroyGlWindow();
				return 0;
			case WM_NCDESTROY:
				appInstance->mainWindow = 0;
				return 0;
			case WM_SIZE:
				if (appInstance->renderer)
					appInstance->renderer->resizeGlScene(LOWORD(lParam),HIWORD(lParam));
				return 0;
			case WM_CREATE:
			case WM_DEVMODECHANGE:
			{
				refreshRate = 60;
				DEVMODE dispSettings;
				memset(&dispSettings,0,sizeof(dispSettings));
				dispSettings.dmSize=sizeof(dispSettings);

				if (EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&dispSettings)){
					refreshRate = dispSettings.dmDisplayFrequency;
				}
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
			{
				if (wParam == VK_RETURN){
					executeAction(cfgEnterKey, appInstance->displayPos->getTarget());
					return 0;
				} else if (wParam == VK_F5){
					bool hardRefresh = (GetKeyState(VK_SHIFT) != 0);
					appInstance->albumCollection->reloadAsynchStart(hardRefresh);
					return 0;
				} else if (wParam == VK_F6){
					appInstance->playbackTracer->moveToNowPlaying();
					return 0;
				} else {
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
				}
				break;;
			}
			case WM_PAINT:
			{
				static double lastTime = 0;
				appInstance->texLoader->blockUpload();
				appInstance->texLoader->runGlDelete();
				appInstance->displayPos->update();
				appInstance->texLoader->setQueueCenter(appInstance->displayPos->getTarget());
				appInstance->renderer->drawFrame();
				/*double curTime = Helpers::getHighresTimer();
				int sleepFor = int((1000.0/(refreshRate*1.10)) - (1000*(curTime - lastTime))); //1.05 are tolerance
				if (sleepFor > 0)
					SleepEx(sleepFor,false);*/
				appInstance->renderer->swapBuffers();
				lastTime = Helpers::getHighresTimer();
				
				if (!appInstance->displayPos->isMoving()){
					appInstance->texLoader->allowUpload();
					ValidateRect(hWnd,NULL);
				}
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
	void executeAction(const char * action, CollectionPos forCover){
		metadb_handle_list tracks = appInstance->albumCollection->getTracks(forCover);
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
	HWND createWindow(HWND parent){
		WNDCLASS	wc = {0};						// Windows Class Structure
		DWORD		dwExStyle;				// Window Extended Style
		DWORD		dwStyle;				// Window Style
		HWND		hWnd;

		//hInstance			= GetModuleHandle(NULL);				// Grab An Instance For Our Window
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
			MessageBox(NULL,L"Failed To Register The Window Class.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
			return 0;											// Return FALSE
		}

		//dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			// Window Extended Style
		dwExStyle = 0;
		//dwStyle=WS_OVERLAPPEDWINDOW;							// Windows Style
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
									(void*)this)))								// Dont Pass Anything To WM_CREATE
		{
			MessageBox(NULL,L"Window Creation Error.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
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

