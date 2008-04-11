#pragma once

class AsynchTexLoader;
class DbAlbumCollection;
class RenderThread;
class DisplayPosition;
class PlaybackTracer;
class ScriptedCoverPositions;

class AppInstance
{
public:
	AppInstance () {
		ZeroMemory(this, sizeof(*this));
	}
public:
	AsynchTexLoader* texLoader;
	DbAlbumCollection* albumCollection;
	RenderThread* renderer;
	DisplayPosition* displayPos;
	PlaybackTracer* playbackTracer;
	ScriptedCoverPositions* coverPos;
	HWND mainWindow;

	inline void redrawMainWin(){
		RedrawWindow(mainWindow,NULL,NULL,RDW_INVALIDATE);
	}
	inline bool isMainWinMinimized(){
		/*PFC_ASSERT(mainWindow);
		HWND root = mainWindow;
		HWND parent;
		for (int i=0; i < 10; i++){
			parent = GetParent(root);
			if (parent)
				root = parent;
		}
		if (parent != 0)
			root = core_api::get_main_window();
		return 0 != IsIconic(root);*/
		return !static_api_ptr_t<ui_control>()->is_visible();
	}
};
