#pragma once

class DbAlbumCollection;
class RenderThread;
class PlaybackTracer;

class AppInstance
{
public:
	std::unique_ptr<DbAlbumCollection> albumCollection;
	std::unique_ptr<RenderThread> renderer;
	std::unique_ptr<PlaybackTracer> playbackTracer;
	HWND mainWindow = nullptr;

	inline void redrawMainWin(){
		RedrawWindow(mainWindow,NULL,NULL,RDW_INVALIDATE);
	}
	inline bool isMainWinMinimized(){
		return !static_api_ptr_t<ui_control>()->is_visible();
	}
};