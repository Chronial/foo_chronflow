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
	bool onChar(WPARAM wParam){
		collection_read_lock lock(appInstance);
		switch (wParam)
		{
		case 1: // any other nonchar character
		case 0x09: // Process a tab. 
			break;

		case 0x08: // Process a backspace. 
			removeChar();
			break;


		case 0x0A: // Process a linefeed. 
		case 0x0D: // Process a carriage return. 
		case 0x1B: // Process an escape. 
			clear();
			break;

		default: // Process any writeable character
			enterChar(wParam);
			break;
		}
		return 0;

	}
	void enterChar(wchar_t c){
		pfc::string8 newString(enteredString);
		newString << pfc::stringcvt::string_utf8_from_wide(&c, 1);
		if (doSearch(newString)){
			enteredString = newString;
		} else {
			MessageBeep(-1);
		}
		lockPlaybackTracer();
		setTimer(typeTimeout);
	}
	void removeChar(){
		enteredString.truncate(enteredString.length() - 1);
		if (enteredString.length() == 0){
			unlockPlaybackTracer();
		} else {
			doSearch(enteredString);
			lockPlaybackTracer();
			setTimer(typeTimeout);
		}
	}
	void clear(){
		unlockPlaybackTracer();
		killTimer();
		clearSearch();
	}
	void timerProc(){
		clear();
	}

private:
	void lockPlaybackTracer(){
		if (!playbackTracerLocked){
			playbackTracerLocked = true;
			appInstance->playbackTracer->lock();
		}
	}
	void unlockPlaybackTracer(){
		if (playbackTracerLocked){
			playbackTracerLocked = false;
			appInstance->playbackTracer->unlock();
		}
	}
	bool doSearch(const char* searchFor){
		//console::print(pfc::string_formatter() << "searching for: " << searchFor);
		ASSERT_SHARED(appInstance->albumCollection);
		CollectionPos pos;
		if (appInstance->albumCollection->performFayt(searchFor, pos)){
			appInstance->albumCollection->setTargetPos(pos);
			return true;
		} else {
			return false;
		}
	}
	void clearSearch(){
		enteredString.reset();
	}
};