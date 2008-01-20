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

#include <windows.h>		// Header File For Windows
#include <windowsX.h>		// Header File For Windows
#include <gl\gl.h>			// Header File For The OpenGL32 Library
#include <gl\glu.h>			// Header File For The GLu32 Library
//#include <gl\glaux.h>		// Header File For The Glaux Library
#include <gl\glext.h>
#include <gl\wglext.h>
#include <math.h>
#include <GdiPlus.h>

#include "resource.h"

// my classes
#include "CollectionPos.h"
#include "Helpers.h"
#include "ImgTexture.h"
#include "ImgFileTexture.h"
#include "TextTexture.h"
#include "AlbumCollection.h"
#include "AsynchTexLoader.h"
#include "DisplayPosition.h"
#include "Renderer.h"
#include "MouseFlicker.h"

#ifdef _DEBUG
#include "Console.h"
#endif

#include "DirAlbumCollection.h"
#include "BrowseAlbumCollection.h"
#include "DbAlbumCollection.h"


#define deg2rad(X) X*0.017453293
#define rad2deg(X) X/0.017453293

// global Variables
extern AsynchTexLoader* gTexLoader;
extern AlbumCollection* gCollection;