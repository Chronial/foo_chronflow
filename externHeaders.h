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

#include "../_sdk/foobar2000/SDK/foobar2000.h"
#include "../_sdk/foobar2000/helpers/helpers.h"
#include "../_sdk/foobar2000/columns_ui/ui_extension.h"


#include <windows.h>		// Header File For Windows
#include <windowsX.h>		// Header File For Windows
#include <gl\gl.h>			// Header File For The OpenGL32 Library
#include <gl\glu.h>			// Header File For The GLu32 Library
//#include <gl\glaux.h>		// Header File For The Glaux Library
#include <gl\glext.h>
#include <gl\wglext.h>
#include <cmath>
#include <GdiPlus.h>