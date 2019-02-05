#pragma once

#include "stdafx.h"
#include "base.h"
#include "config.h"

#include "Helpers.h"


class FindAsYouType {
	inline static const double typeTimeout = 1.0;
	pfc::string8 enteredString;
	std::optional<Timer> timeoutTimer;

	class Engine& engine;
public:
	FindAsYouType(Engine& engine)
		: engine(engine) {};
	bool onChar(WPARAM wParam);

private:
	void enterChar(wchar_t c);
	void removeChar();
	void reset();
	bool updateSearch(const char* searchFor);
};