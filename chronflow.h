#pragma once

#include "../_sdk/foobar2000/SDK/foobar2000.h"
#include "../_sdk/foobar2000/helpers/helpers.h"
#include "../_sdk/foobar2000/columns_ui/ui_extension.h"

#ifndef WINVER
#define WINVER 0x0500
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif                       

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0500
#endif

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

#include <windows.h>		// Header File For Windows
#include <windowsX.h>		// Header File For Windows
#include <gl\gl.h>			// Header File For The OpenGL32 Library
#include <gl\glu.h>			// Header File For The GLu32 Library
//#include <gl\glaux.h>		// Header File For The Glaux Library
#include <gl\glext.h>
#include <gl\wglext.h>
#include <cmath>
#include <GdiPlus.h>

#include "ScriptObject.h"
#include "SafeArrayHelper.h"

#include "resource.h"

// my classes
#include "cfg_coverConfigs.h"

class AppInstance;
#include "CollectionPos.h"
#include "Renderer.h"
#include "Helpers.h"

#include "ImgTexture.h"
#include "AlbumCollection.h"
#include "AsynchTexLoader.h"
#include "DisplayPosition.h"
#include "TextDisplay.h"
#include "MouseFlicker.h"
#include "PlaybackTracer.h"
#include "AppInstance.h"

#include "ScriptedCoverPositions.h"


#ifdef _DEBUG
#include "Console.h"
#endif

#include "MyActions.h"

#include "DbAlbumCollection.h"


// this links the ConfigWindow to the Single Instance
AppInstance* gGetSingleInstance();

// {37835416-4578-4aaa-A229-E09AB9E2CB9C}
const GUID guid_configWindow = { 0x37835416, 0x4578, 0x4aaa, { 0xa2, 0x29, 0xe0, 0x9a, 0xb9, 0xe2, 0xcb, 0x9c } };
