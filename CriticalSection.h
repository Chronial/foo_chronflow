#pragma once
#include "stdafx.h"
#include "base.h"


class CriticalSection {
private:
	CRITICAL_SECTION sec;
	IF_DEBUG(DWORD holdingThread);
public:
	void enter() throw();
	void leave() throw();
#ifdef _DEBUG
	void assertOwnage();
#endif
	CriticalSection();
	~CriticalSection();
};
