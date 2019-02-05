#include "stdafx.h"
#include "base.h"
#include "ChronflowWindow.h"

#include "Console.h"
#include "RenderThread.h"
#include "RenderWindow.h"
#include "Engine.h"

#define MAINWINDOW_CLASSNAME L"foo_chronflow MainWindow"

#define MINIMIZE_CHECK_TIMEOUT 10000 // milliseconds

ChronflowWindow::ChronflowWindow(HWND parent, ui_element_instance_callback_ptr duiCallback) {
	TRACK_CALL_TEXT("ChronflowWindow::startup");
	IF_DEBUG(Console::create());

	hwnd = createWindow(parent);
	try {
		renderWindow.emplace(hwnd, duiCallback);
	} catch (std::runtime_error&){
		// We live on without the render window, so we can display the error
	}
}

ChronflowWindow::~ChronflowWindow() {
	DestroyWindow(hwnd);
}

bool ChronflowWindow::registerWindowClass(){
	HINSTANCE myInstance = core_api::get_my_instance();

	WNDCLASS wc = { 0 };
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS | CS_NOCLOSE;
	wc.lpfnWndProc = (WNDPROC)ChronflowWindow::WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = sizeof(ChronflowWindow *);
	wc.hInstance = myInstance;
	wc.hIcon = LoadIcon(NULL, IDI_HAND);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = MAINWINDOW_CLASSNAME;

	return RegisterClass(&wc) != 0;
}

LRESULT ChronflowWindow::MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	switch (uMsg){
	case WM_SIZE:
	{
		if (renderWindow){
			renderWindow->setWindowSize(LOWORD(lParam), HIWORD(lParam));
		}
		return 0;
	}
	case WM_DISPLAYCHANGE:
	case WM_DEVMODECHANGE:
	{
		if (renderWindow)
			renderWindow->renderThread->send<EM::DeviceModeMessage>();
		return 0;
	}
	case WM_TIMER:
		switch (wParam){
		case IDT_CHECK_MINIMIZED:
			if (!static_api_ptr_t<ui_control>()->is_visible()){
				if (!mainWinMinimized){
					mainWinMinimized = true;
					if (renderWindow)
						renderWindow->renderThread->send<EM::WindowHideMessage>();
					KillTimer(hWnd, IDT_CHECK_MINIMIZED);
				}
			}
			break;
		}
		return 0;
	case WM_ERASEBKGND:
		return TRUE;
	case WM_PAINT:
	{
		if (GetUpdateRect(hWnd, 0, FALSE)){
			if (renderWindow){
				renderWindow->onDamage();
			} else {
				PAINTSTRUCT ps;
				HDC hdc;
				RECT rc;
				hdc = BeginPaint(hWnd, &ps);
				GetClientRect(hWnd, &rc);
				FillRect(hdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
				rc.top += 10;
				DrawText(hdc, L"foo_chronflow failed to open an opengl window :(.\nSee Console for details.", -1,
					&rc, DT_CENTER | DT_VCENTER);
				EndPaint(hWnd, &ps);
				return 0;
			}
			if (mainWinMinimized){
				mainWinMinimized = false;
				if (renderWindow)
					renderWindow->renderThread->send<EM::WindowShowMessage>();
			}
			SetTimer(hWnd, IDT_CHECK_MINIMIZED, MINIMIZE_CHECK_TIMEOUT, 0);
		}
	}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK ChronflowWindow::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	ChronflowWindow* chronflow = nullptr;
	if (uMsg == WM_NCCREATE){
		chronflow = reinterpret_cast<ChronflowWindow*>(((CREATESTRUCT*)lParam)->lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)chronflow);
	} else {
		chronflow = reinterpret_cast<ChronflowWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	}
	if (chronflow == 0)
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	return chronflow->MessageHandler(hWnd, uMsg, wParam, lParam);
}

HWND  ChronflowWindow::createWindow(HWND parent){
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
};
