#pragma once
#ifndef WINVER
#define WINVER 0x0500
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif                       

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0500
#endif

#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"

#include <windows.h>		// Header File For Windows
#include <windowsX.h>		// Header File For Windows

#include <gl\gl.h>			// Header File For The OpenGL32 Library
#include <gl\glu.h>			// Header File For The GLu32 Library
#include "opengl\glext.h"
#include "opengl\wglext.h"
#include <cmath>
#include <GdiPlus.h>

// copied from colums_ui
#include "win32_helpers.h"