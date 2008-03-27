#pragma once

class AsynchTexLoader;
class DbAlbumCollection;
class Renderer;
class DisplayPosition;
class PlaybackTracer;
class ScriptedCoverPositions;

class AppInstance
{
public:
	AppInstance ()
		: texLoader(0), albumCollection(0), renderer(0), textDisplay(0),
		displayPos(0), playbackTracer(0), coverPos(0), mainWindow(0) {}
public:
	AsynchTexLoader* texLoader;
	DbAlbumCollection* albumCollection;
	Renderer* renderer;
	TextDisplay* textDisplay;
	DisplayPosition* displayPos;
	PlaybackTracer* playbackTracer;
	ScriptedCoverPositions* coverPos;
	HWND mainWindow;
};
