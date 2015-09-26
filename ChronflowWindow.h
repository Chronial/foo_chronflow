#pragma once
#include "stdafx.h"

class AppInstance;

class ChronflowWindow {
	ULONG_PTR gdiplusToken;
	bool mainWinMinimized = true;

public:
	static std::unordered_set<ChronflowWindow*> instances;
	ChronflowWindow(){}
	virtual ~ChronflowWindow();

	virtual ui_element_instance_callback_ptr getDuiCallback(){ return nullptr; }
	static bool registerWindowClass();

	AppInstance* appInstance = nullptr;
protected:
	void startup(HWND parent);

private:
	HWND createWindow(HWND parent);

	// Called on window destruction
	void shutdown();

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};