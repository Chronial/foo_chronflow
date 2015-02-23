#pragma once

#define _WIN32_WINNT 0x501
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <ActivScp.h>
#include <Commdlg.h>
#include <GdiPlus.h>
#include <ShellApi.h>
#include <Mmsystem.h>

#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"

#include <gl\gl.h>
#include <gl\glu.h>
#include "opengl\glext.h"
#include "opengl\wglext.h"
#include <cmath>

// copied from colums_ui TODO: strip this down
#include "win32_helpers.h"