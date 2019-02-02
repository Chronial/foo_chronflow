#pragma once

#include "stdafx.h"
#include "base.h"
#include "config.h"

#include "AppInstance.h"
#include "DbAlbumCollection.h"
#include "PlaybackTracer.h"
#include "TimerOwner.h"


class FindAsYouType : TimerOwner {
	static const int typeTimeout = 1000; // milliseconds
	pfc::string8 enteredString;
	AppInstance* appInstance;
	bool playbackTracerLocked;
public:
	FindAsYouType(AppInstance* instance)
		: TimerOwner(instance), appInstance(instance){
		clearSearch();
		playbackTracerLocked = false;
	}
	bool onChar(WPARAM wParam);
	void enterChar(wchar_t c);
	void removeChar();
	void clear();
	void timerProc();

private:
	void lockPlaybackTracer();
	void unlockPlaybackTracer();
	bool doSearch(const char* searchFor);
	void clearSearch();
};