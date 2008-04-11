#pragma once
#include "externHeaders.h"

#ifdef _DEBUG
#define IF_DEBUG(X) X
#else
#define IF_DEBUG(X)  
#endif

#ifndef M_PI
	#define M_PI 3.1415926535897932385
#endif
#define deg2rad(X) (X*M_PI/180)
#define rad2deg(X) (X*180/M_PI)



#include "ScriptObject.h"
#include "SafeArrayHelper.h"

#include "resource.h"

#define WM_COLLECTION_REFRESHED (WM_USER+1)

#define IDT_CHECK_MINIMIZED 665
#define IDT_PLAYBACK_TRACER 666
#define IDT_FIND_AS_YOU_TYPE_RESET 667

// my classes
#ifdef _DEBUG
#include "Console.h"
#endif

#include "cfg_coverConfigs.h"

#include "Helpers.h"
#include "AppInstance.h"
#include "AlbumCollection.h"
#include "DbAlbumCollection.h"
#include "CollectionPos.h"
#include "TextDisplay.h"
#include "Renderer.h"
#include "RenderThread.h"

#include "ImgTexture.h"
#include "AsynchTexLoader.h"
#include "DisplayPosition.h"
#include "MouseFlicker.h"
#include "PlaybackTracer.h"

#include "ScriptedCoverPositions.h"
#include "MyActions.h"



// this links the ConfigWindow to the Single Instance
AppInstance* gGetSingleInstance();

// {37835416-4578-4aaa-A229-E09AB9E2CB9C}
const GUID guid_configWindow = { 0x37835416, 0x4578, 0x4aaa, { 0xa2, 0x29, 0xe0, 0x9a, 0xb9, 0xe2, 0xcb, 0x9c } };
