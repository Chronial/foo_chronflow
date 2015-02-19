#include "stdafx.h"
#include "base.h"

#include "CriticalSection.h"

void CriticalSection::enter() throw() {
	EnterCriticalSection(&sec);
	IF_DEBUG(holdingThread = GetCurrentThreadId());
}

void CriticalSection::leave() throw() {
	IF_DEBUG(holdingThread = 0);
	LeaveCriticalSection(&sec);
}

#ifdef _DEBUG
void CriticalSection::assertOwnage() {
	PFC_ASSERT(holdingThread == GetCurrentThreadId());
}
#endif

CriticalSection::CriticalSection() { InitializeCriticalSectionAndSpinCount(&sec, 0x80000400); }
CriticalSection::~CriticalSection() { DeleteCriticalSection(&sec); }