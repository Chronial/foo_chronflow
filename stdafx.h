#pragma once

#define _WIN32_WINNT 0x501
#define WIN32_LEAN_AND_MEAN

#define GLFW_INCLUDE_GLU
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <windows.h>
#include <ActivScp.h>
#include <Commdlg.h>
#include <GdiPlus.h>
#include <ShellApi.h>
#include <Mmsystem.h>
#include <Shlwapi.h>
#include <process.h>


// Get rid of these macros
#undef min
#undef max

#include "../foobar2000/SDK/foobar2000.h"
#include "../foobar2000/helpers/helpers.h"


//#define APIENTRY
#include "opengl\glext.h"
#include "opengl\wglext.h"

#include <deque>
#include <unordered_set>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <atomic>
#include <thread>

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
#include <boost/thread/synchronized_value.hpp>

#include <boost/optional.hpp>

using boost::shared_lock;

#ifdef _DEBUG
#define BOOST_MULTI_INDEX_ENABLE_SAFE_MODE
#endif

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ranked_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>


#include <gsl/gsl>
