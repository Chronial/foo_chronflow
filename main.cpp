#include "chronflow.h"
#include "config.h"

#define VERSION "0.1.1"

DECLARE_COMPONENT_VERSION( "Coverflow pannel", VERSION, 
   "Renders Album Art in a 3d environment\n"
   "By Christian Fersch\n"
   __DATE__ " - " __TIME__ );


AsynchTexLoader* gTexLoader = 0;
AlbumCollection* gCollection = 0;

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
		return ui_extension::window_type_t::type_panel;
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
	bool disabled;
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
	}
	void init(){
	}
	void quit(){
	}
	void show(HWND parent){
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

		if (!Helpers::isPerformanceCounterSupported()){
			TIMECAPS tc;
			UINT     wTimerRes;
			if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) == TIMERR_NOERROR){
				wTimerRes = min(max(tc.wPeriodMin, 1), tc.wPeriodMax);
				timeBeginPeriod(wTimerRes); 
			}
		}
		
#ifdef _DEBUG
		Console::create();
#endif
		//gCollection = new DirAlbumCollection();
		//gCollection = new BrowseAlbumCollection();
		disabled = false;
		gCollection = new DbAlbumCollection();
		if (gCollection->getCount() == 0)
			disabled = true;
		gTexLoader = new AsynchTexLoader(gCollection);
		createWindow(parent);
		displayPos = new DisplayPosition(gCollection, CollectionPos(gCollection,0), hWnd);
		renderer = new Renderer(displayPos);
		mouseFlicker = new MouseFlicker(displayPos);

		if (!renderer->setupGlWindow(hWnd))
			return;
		gTexLoader->setNotifyWindow(hWnd);
		setupDone = true;
	}
	void hide(){
		setupDone = false;
		delete mouseFlicker;
		renderer->destroyGlWindow();
		delete renderer;
		delete displayPos;
		destroyWindow();
		delete gTexLoader;
		delete gCollection;

		if (multimediaTimerRes != 0)
			timeEndPeriod(multimediaTimerRes);

		Gdiplus::GdiplusShutdown(gdiplusToken);
	}
private:
	LRESULT MessageHandler (HWND	hWnd,			// Handle For This Window
							UINT	uMsg,			// Message For This Window
							WPARAM	wParam,			// Additional Message Information
							LPARAM	lParam){
		if (disabled && uMsg != WM_CLOSE &&
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
		}
		switch (uMsg){
			case WM_CLOSE:
				PostQuitMessage(0);
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
			case WM_LBUTTONDOWN:
			case WM_LBUTTONDBLCLK:
			{
				CollectionPos newTarget = displayPos->getCenteredPos();
				if (renderer->positionOnPoint(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam),&newTarget)){
					displayPos->setTarget(newTarget);
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
				}
				return 0;
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
	void destroyWindow(){
		if (hWnd && !DestroyWindow(hWnd))					// Are We Able To Destroy The Window?
		{
			MessageBox(NULL,L"Could Not Release hWnd.",L"SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
			hWnd=NULL;										// Set hWnd To NULL
		}

		if (!UnregisterClass(L"Chronflow",core_api::get_my_instance()))			// Are We Able To Unregister Class
		{
			MessageBox(NULL,L"Could Not Unregister Class.",L"SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
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
									/**NULL*/ parent,								// No Parent Window
									NULL,								// No Menu
									/**hInstance*/ core_api::get_my_instance(),							// Instance
									(void*)this)))								// Dont Pass Anything To WM_CREATE
		{
			destroyWindow();								// Reset The Display
			MessageBox(NULL,L"Window Creation Error.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
			return FALSE;								// Return FALSE
		}
		return TRUE;
	}
};

static ui_extension::window_factory_single< Chronflow > x_chronflow;

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