#pragma once

#pragma warning(push, 1)
#pragma warning(disable : 4068)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"

#include <array>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <future>
#include <iomanip>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>

using std::make_shared;  // NOLINT
using std::make_unique;  // NOLINT
using std::shared_ptr;  // NOLINT
using std::unique_ptr;  // NOLINT

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

#include <boost/algorithm/string/replace.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/combine.hpp>
#include <boost/range/sub_range.hpp>

// Require windows vista
#define _WIN32_WINNT 0x600
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOSOUND
#define NOKANJI
#define NOKERNEL
#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NORASTEROPS
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOMEMMGR
#define NOOPENFILE
#define NOSCROLL
//#define NOTEXTMETRIC
#define NOWH
#define NOCOMM
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX
#include <windows.h>

#include <ActivScp.h>
#include <Shlwapi.h>
#include <commdlg.h>
#include <gdiplus.h>
#include <mmsystem.h>
#include <process.h>
#include <shellapi.h>
#undef min
#undef max

#include "lib/glad.h"
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#define APIENTRY WINAPI

#include "../foobar2000/SDK/foobar2000.h"
#include "../foobar2000/helpers/win32_misc.h"
#include "../pfc/range_based_for.h"

// clang doesn't support stdcall lambdas
// since we only use it for linting, we just hide those lambdas from it
#ifdef __clang__
#define WINLAMBDA(X) nullptr
#else
#define WINLAMBDA(X) X
#endif

#pragma clang diagnostic pop
#pragma clang diagnostic ignored "-Wmicrosoft-enum-forward-reference"
#pragma clang diagnostic ignored "-Wwritable-strings"

#pragma warning(pop)
// 4100: Unused function argument
// 4201: nonstandard extension used : nameless struct/union
// 4458: Declaration hides class member
#pragma warning(disable : 4100 4201 4458)
