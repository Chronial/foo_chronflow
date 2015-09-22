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
		texLoader = 0;
		albumCollection = 0;
		renderer = 0;
		playbackTracer = 0;
		mainWindow = 0;
	}

	AsynchTexLoader* texLoader;
	DbAlbumCollection* albumCollection;
	RenderThread* renderer;
	PlaybackTracer* playbackTracer;
	HWND mainWindow;

	inline void redrawMainWin(){
		RedrawWindow(mainWindow,NULL,NULL,RDW_INVALIDATE);
	}
	inline bool isMainWinMinimized(){
		return !static_api_ptr_t<ui_control>()->is_visible();
	}
};