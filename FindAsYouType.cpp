#include "stdafx.h"
#include "FindAsYouType.h"
#include "EngineThread.h"
#include "DbAlbumCollection.h"
#include "Engine.h"
#include "PlaybackTracer.h"


bool FindAsYouType::onChar(WPARAM wParam) {
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
		reset();
		break;

	default: // Process any writeable character
		enterChar(static_cast<wchar_t>(wParam));
		break;
	}
	return 0;

}

void FindAsYouType::enterChar(wchar_t c) {
	pfc::string8 newString(enteredString);
	newString << pfc::stringcvt::string_utf8_from_wide(&c, 1);
	if (updateSearch(newString)) {
		enteredString = newString;
	} else {
		MessageBeep(0xFFFFFFFF);
	}
}

void FindAsYouType::removeChar() {
	enteredString.truncate(enteredString.length() - 1);
	if (enteredString.length() > 0) {
		updateSearch(enteredString);
	}
}

void FindAsYouType::reset() {
	timeoutTimer.reset();
	enteredString.reset();
}

bool FindAsYouType::updateSearch(const char * searchFor) {
	//console::print(pfc::string_formatter() << "searching for: " << searchFor);
	timeoutTimer.reset();
	engine.playbackTracer.delay(typeTimeout);
	CollectionPos pos;
	bool result = engine.db.performFayt(searchFor, pos);
	if (result) {
		engine.db.setTargetPos(pos);
		engine.onTargetChange(true);
	}

	timeoutTimer.emplace(typeTimeout, [&] {
		engine.thread.send<EM::Run>([&] {
			reset();
		});
	});

	return result;
}
