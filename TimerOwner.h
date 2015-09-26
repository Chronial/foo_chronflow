#pragma once

#include "stdafx.h"
#include "AppInstance.h"

// Classes inheriting from this must be destroyed before the appInstance mainWindow
class TimerOwner {
	AppInstance* appInstance;
public:
	TimerOwner(AppInstance* appInstance) : appInstance(appInstance) {}
	~TimerOwner(){
		killTimer();
	}
	void killTimer(){
		KillTimer(appInstance->mainWindow, reinterpret_cast<UINT_PTR>(this));
	}
	void setTimer(UINT uElapse){
		SetTimer(appInstance->mainWindow, reinterpret_cast<UINT_PTR>(this), uElapse, &TimerOwner::TimerProc);
	}
	static VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime){
		reinterpret_cast<TimerOwner*>(idEvent)->timerProc();
	}
	virtual void timerProc() = 0;
};
