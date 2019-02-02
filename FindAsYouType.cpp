#include "stdafx.h"
#include "FindAsYouType.h"
#include "RenderThread.h"


bool FindAsYouType::onChar(WPARAM wParam) {
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

void FindAsYouType::enterChar(wchar_t c) {
	pfc::string8 newString(enteredString);
	newString << pfc::stringcvt::string_utf8_from_wide(&c, 1);
	if (doSearch(newString)) {
		enteredString = newString;
	} else {
		MessageBeep(-1);
	}
	lockPlaybackTracer();
	setTimer(typeTimeout);
}

void FindAsYouType::removeChar() {
	enteredString.truncate(enteredString.length() - 1);
	if (enteredString.length() == 0) {
		unlockPlaybackTracer();
	} else {
		doSearch(enteredString);
		lockPlaybackTracer();
		setTimer(typeTimeout);
	}
}

void FindAsYouType::clear() {
	unlockPlaybackTracer();
	killTimer();
	clearSearch();
}

void FindAsYouType::timerProc() {
	appInstance->renderer->send<RenderThread::Run>([&] {
		clear();
	});
}

void FindAsYouType::lockPlaybackTracer() {
	if (!playbackTracerLocked) {
		playbackTracerLocked = true;
		appInstance->playbackTracer->lock();
	}
}

void FindAsYouType::unlockPlaybackTracer() {
	if (playbackTracerLocked) {
		playbackTracerLocked = false;
		appInstance->playbackTracer->unlock();
	}
}

bool FindAsYouType::doSearch(const char * searchFor) {
	//console::print(pfc::string_formatter() << "searching for: " << searchFor);
	ASSERT_SHARED(appInstance->albumCollection);
	CollectionPos pos;
	if (appInstance->albumCollection->performFayt(searchFor, pos)) {
		appInstance->albumCollection->setTargetPos(pos);
		return true;
	} else {
		return false;
	}
}

void FindAsYouType::clearSearch() {
	enteredString.reset();
}
