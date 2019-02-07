#pragma once
#pragma warning(push, 1)


#include <array>
#include <deque>
#include <limits>
#include <unordered_set>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <atomic>
#include <thread>
#include <future>
#include <shared_mutex>

using std::make_shared;
using std::make_unique;
using std::shared_ptr;
using std::unique_ptr;


#include <gsl/gsl>


#ifdef _DEBUG
#define BOOST_MULTI_INDEX_ENABLE_SAFE_MODE
#endif
#define BOOST_DETAIL_NO_CONTAINER_FWD
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/ranked_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>


// Require windows vista
#define _WIN32_WINNT 0x600
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <ActivScp.h>
#include <Commdlg.h>
#include <GdiPlus.h>
#include <Mmsystem.h>
#include <ShellApi.h>
#include <Shlwapi.h>
#include <Process.h>
#undef min
#undef max


#include "lib/glad.h"
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#define APIENTRY WINAPI


#include "../foobar2000/SDK/foobar2000.h"
#include "../foobar2000/helpers/helpers.h"


#pragma warning(pop)
// 4100: Unused function argument
// 4201: nonstandard extension used : nameless struct/union
// 4458: Declaration hides class member
#pragma warning(disable: 4100 4201 4458)
