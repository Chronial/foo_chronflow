#pragma once
#include "stdafx.h"
#include "EngineWindow.h"

struct GdiContext {
	ULONG_PTR token;
	GdiContext() {
		Gdiplus::GdiplusStartupInput input;
		Gdiplus::GdiplusStartup(&token, &input, nullptr);
	}
	~GdiContext() {
		Gdiplus::GdiplusShutdown(token);
	}
};

class ContainerWindow {
	GdiContext gdiContext;
	bool mainWinMinimized = true;

public:
	HWND hwnd = nullptr;

	ContainerWindow(HWND parent, ui_element_instance_callback_ptr duiCallback=nullptr);
	~ContainerWindow();

	static bool registerWindowClass();

	std::optional<EngineWindow> engineWindow;

private:
	HWND createWindow(HWND parent);

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};