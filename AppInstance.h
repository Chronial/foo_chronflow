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
		return !static_api_ptr_t<ui_control>()->is_visible();
	}
};
