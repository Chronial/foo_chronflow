#pragma once

#pragma warning(push, 1)

// Require windows vista
#define _WIN32_WINNT 0x600
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


#include "opengl\glext.h"

#include <array>
#include <deque>
#include <unordered_set>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <atomic>
#include <thread>
#include <future>
#include <shared_mutex>

using std::unique_ptr;
using std::make_unique;
using std::shared_ptr;
using std::make_shared;

// copied from colums_ui TODO: strip this down
#include "win32_helpers.h"

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


#pragma warning(pop)
// 4100: Unused function argument
// 4201: nonstandard extension used : nameless struct/union
// 4458: Declaration hides class member
#pragma warning(disable: 4100 4201 4458)
