#pragma once
#include "stdafx.h"
#include "RenderWindow.h"

struct GdiContext {
	ULONG_PTR token;
	GdiContext() {
		Gdiplus::GdiplusStartupInput input;
		Gdiplus::GdiplusStartup(&token, &input, NULL);
	}
	~GdiContext() {
		Gdiplus::GdiplusShutdown(token);
	}
};

class ChronflowWindow {
	GdiContext gdiContext;
	bool mainWinMinimized = true;

public:
	HWND hwnd = nullptr;

	ChronflowWindow(HWND parent, ui_element_instance_callback_ptr duiCallback=nullptr);
	~ChronflowWindow();

	static bool registerWindowClass();

	std::optional<RenderWindow> renderWindow;

private:
	HWND createWindow(HWND parent);

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};