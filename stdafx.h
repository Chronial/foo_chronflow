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

#include <deque>
#include <unordered_set>
#include <condition_variable>
#include <mutex>
#include <memory>

using std::unique_ptr;
using std::make_unique;
using std::shared_ptr;
using std::make_shared;
using std::dynamic_pointer_cast;

// copied from colums_ui TODO: strip this down
#include "win32_helpers.h"

#include <boost/thread/lock_types.hpp> 
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>


using boost::shared_mutex;
using boost::shared_lock;