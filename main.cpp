#include "chronflow.h"
#include "config.h"

#define VERSION "0.1.1"

DECLARE_COMPONENT_VERSION( "Coverflow pannel", VERSION, 
   "Renders Album Art in a 3d environment\n"
   "By Christian Fersch\n"
   __DATE__ " - " __TIME__ );


AsynchTexLoader* gTexLoader = 0;
AlbumCollection* gCollection = 0;

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
			ShowWindow(hWnd, SW_HIDE);
			SetParent(hWnd, wnd_parent);
			SetWindowPos(hWnd, NULL, p_position.x, p_position.y, p_position.cx, p_position.cy, SWP_NOZORDER);
			currentHost->relinquish_ownership(hWnd);
		} else {
			show(wnd_parent);
			SetWindowPos(hWnd, NULL, p_position.x, p_position.y, p_position.cx, p_position.cy, SWP_NOZORDER);
			ShowWindow(hWnd, SW_HIDE);
		}
		currentHost = p_host;
		isShown = true;
		return hWnd;
	};

	void destroy_window() {
		if (isShown){
			hide();
			currentHost = 0;
			isShown = false;
		}
	};

	HWND get_wnd() const {
		return hWnd;
	};

private:
	bool isShown;
	bool setupDone;
	//bool disabled;
	Renderer* renderer;
	DisplayPosition* displayPos;
	MouseFlicker* mouseFlicker;
	HWND hWnd;
	int refreshRate;
	ULONG_PTR gdiplusToken;
	UINT multimediaTimerRes;
public:
	Chronflow(){
		multimediaTimerRes = 0;
		currentHost = 0;
		setupDone = false;
		isShown = false;
		renderer = 0;
	}
	void init(){
		if (!Helpers::isPerformanceCounterSupported()){
			TIMECAPS tc;
			UINT     wTimerRes;
			if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) == TIMERR_NOERROR){
				wTimerRes = min(max(tc.wPeriodMin, 1), tc.wPeriodMax);
				timeBeginPeriod(wTimerRes); 
			}
		}
	}
	void quit(){
		if (multimediaTimerRes != 0)
			timeEndPeriod(multimediaTimerRes);
	}
	void show(HWND parent){
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

#ifdef _DEBUG
		Console::create();
#endif
		createWindow(parent);
		gCollection = new DbAlbumCollection();
		//if (gCollection->getCount() == 0)
		//	disabled = true;
		gTexLoader = new AsynchTexLoader(gCollection);
		displayPos = new DisplayPosition(gCollection, CollectionPos(gCollection,sessionSelectedCover), hWnd);
		renderer = new Renderer(displayPos);
		mouseFlicker = new MouseFlicker(displayPos);

		if (!renderer->setupGlWindow(hWnd))
			return;
		gTexLoader->setNotifyWindow(hWnd);
		setupDone = true;
		gCollection->reloadAsynchStart(hWnd, true);
	}
	void hide(){
		sessionSelectedCover = displayPos->getCenteredPos().toIndex();
		setupDone = false;
		delete mouseFlicker;
		if (hWnd)
			DestroyWindow(hWnd);
		UnregisterClass(L"Chronflow",core_api::get_my_instance());
		delete renderer;
		renderer = 0;
		delete displayPos;
		delete gTexLoader;
		delete gCollection;

		Gdiplus::GdiplusShutdown(gdiplusToken);
	}
	HWND getHWnd(){
		return hWnd;
	}
private:
	LRESULT MessageHandler (HWND	hWnd,			// Handle For This Window
							UINT	uMsg,			// Message For This Window
							WPARAM	wParam,			// Additional Message Information
							LPARAM	lParam){
		if(renderer)
			renderer->setRenderingContext();

		/*if (disabled && uMsg != WM_CLOSE &&
						uMsg != WM_SIZE &&
						uMsg != WM_CREATE &&
						uMsg != WM_DEVMODECHANGE){
			if (uMsg == WM_PAINT){
				renderer->drawEmptyFrame();
				renderer->swapBuffers();
				ValidateRect(hWnd,NULL);
				return 0;
			} else {
				return DefWindowProc(hWnd,uMsg,wParam,lParam);
			}
		}*/
		switch (uMsg){
			case WM_COLLECTION_REFRESHED:
				gCollection->reloadAsynchFinish(lParam, displayPos);
				return 0;
			case WM_DESTROY:
				if (renderer)
					renderer->destroyGlWindow();
				return 0;
			case WM_NCDESTROY:
				this->hWnd = 0;
				return 0;
			case WM_SIZE:
				if (setupDone)
					renderer->resizeGlScene(LOWORD(lParam),HIWORD(lParam));
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
				displayPos->setTarget(displayPos->getTarget() + m);
				return 0;
			}
			case WM_MBUTTONDOWN:
			{
				CollectionPos newTarget = displayPos->getCenteredPos();
				if (renderer->positionOnPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), newTarget)){
					displayPos->setTarget(newTarget);
					executeAction(cfgMiddleClick, newTarget);
				}
				return 0;
			}
			case WM_LBUTTONDBLCLK:
			case WM_LBUTTONDOWN:
			{
				CollectionPos newTarget = displayPos->getCenteredPos();
				if (renderer->positionOnPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), newTarget)){
					displayPos->setTarget(newTarget);
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
			case WM_KEYDOWN:
			{
				if (wParam == VK_RETURN){
					executeAction(cfgEnterKey, displayPos->getTarget());
					return 0;
				} else if (wParam == VK_F5){
					bool hardRefresh = (GetKeyState(VK_SHIFT) != 0);
					gCollection->reloadAsynchStart(hWnd, hardRefresh);
					return 0;
				} else if (wParam == VK_F6){
					static_api_ptr_t<playback_control_v2> pc;
					metadb_handle_ptr nowPlaying;
					if (pc->get_now_playing(nowPlaying)){
						CollectionPos target = displayPos->getTarget();
						if (gCollection->getAlbumForTrack(nowPlaying, target)){
							displayPos->setTarget(target);
						}
					}
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
						displayPos->setTarget(displayPos->getTarget() + move);
						return 0;
					}
				}
				break;;
			}
			case WM_PAINT:
			{
				static double lastTime = 0;
				gTexLoader->runGlDelete();
				displayPos->update();
				gTexLoader->setQueueCenter(displayPos->getTarget());
				renderer->drawFrame();
				/*double curTime = Helpers::getHighresTimer();
				int sleepFor = int((1000.0/(refreshRate*1.10)) - (1000*(curTime - lastTime))); //1.05 are tolerance
				if (sleepFor > 0)
					SleepEx(sleepFor,false);*/
				renderer->swapBuffers();
				lastTime = Helpers::getHighresTimer();
#ifdef _DEBUG
				Helpers::FPS(hWnd,displayPos->getCenteredPos(),displayPos->getCenteredOffset());
#endif
				
				if (!displayPos->isMoving()){
					if(gTexLoader->runGlUpload(5))
						ValidateRect(hWnd,NULL);
				}
				return 0;
			}
		}
		return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}
	static LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		static Chronflow* chronflow;
		if (uMsg == WM_NCCREATE)
			chronflow = (Chronflow*) ((CREATESTRUCT*)lParam)->lpCreateParams;
		//Chronflow* chronflow = reinterpret_cast<Chronflow *>(GetWindowLongPtr(hWnd, 0));

		return chronflow->MessageHandler(hWnd, uMsg, wParam, lParam);
	}
	void executeAction(const char * action, CollectionPos forCover){
		metadb_handle_list tracks = gCollection->getTracks(forCover);
		const char * albumTitle = gCollection->getTitle(forCover);
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
	bool createWindow(HWND parent){
		WNDCLASS	wc;						// Windows Class Structure
		DWORD		dwExStyle;				// Window Extended Style
		DWORD		dwStyle;				// Window Style

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
			return FALSE;											// Return FALSE
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
			return FALSE;								// Return FALSE
		}
		return TRUE;
	}
};

static ui_extension::window_factory_single< Chronflow > x_chronflow;

HWND gGetMainWindow(){
	return x_chronflow.get_static_instance().getHWnd();
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

