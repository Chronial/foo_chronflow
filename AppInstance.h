#pragma once
#include "DbReloadWorker.h"

class DbAlbumCollection;
class DbReloadWorker;
class RenderThread;
class RenderWindow;
class PlaybackTracer;

class AppInstance
{
public:
	std::unique_ptr<DbAlbumCollection> albumCollection;
	std::unique_ptr<RenderWindow> renderWindow;
	std::unique_ptr<RenderThread> renderer;
	std::unique_ptr<PlaybackTracer> playbackTracer;
	boost::synchronized_value<std::unique_ptr<DbReloadWorker>> reloadWorker;

	// TODO: Get rid of this pointer?
	GLFWwindow* glfwWindow = nullptr;
	HWND mainWindow = nullptr;

	inline void redrawMainWin(){
		RedrawWindow(mainWindow,NULL,NULL,RDW_INVALIDATE);
	}
	inline void startCollectionReload(){
		DbReloadWorker::startNew(this);
	}
};