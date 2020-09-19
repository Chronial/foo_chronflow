#pragma once
#include "resource.h"

#ifdef _DEBUG
#define IF_DEBUG(X) X
#else
#define IF_DEBUG(X)
#endif

#ifndef M_PI
#define M_PI 3.1415926535897932385
#endif
constexpr double deg2rad(double deg) {
  return deg * M_PI / 180;
};
constexpr double rad2deg(double rad) {
  return rad * 180 / M_PI;
};

#define NO_MOVE_NO_COPY(C) \
  C(const C&) = delete; \
  C& operator=(const C&) = delete; \
  C(C&&) = delete; \
  C& operator=(C&&) = delete;

// {37835416-4578-4aaa-A229-E09AB9E2CB9C}
const GUID guid_configWindow = {
    0x37835416, 0x4578, 0x4aaa, {0xa2, 0x29, 0xe0, 0x9a, 0xb9, 0xe2, 0xcb, 0x9c}};

template <auto fn>
using fn_class = std::integral_constant<decltype(fn), fn>;

template <typename T, auto fn>
using unique_ptr_del = std::unique_ptr<T, fn_class<fn>>;

struct ILessUtf8 {
  bool operator()(const std::string& a, const std::string& b) const {
    return stricmp_utf8(a.c_str(), b.c_str()) < 0;
  }
};

extern const char** builtInCoverConfigArray;
constexpr char* defaultCoverConfig = "Default (build-in)";
constexpr char* coverConfigTemplate = "Template (build-in)";

#define TEST_BIT_PTR(BITSET_PTR, BIT) _bittest(BITSET_PTR, BIT)
#ifdef _DEBUG
#define ASSUME(X) PFC_ASSERT(X)
#else
#define ASSUME(X) __assume(X)
#endif

void errorPopup(const char* message);
void errorPopupWin32(
    const char* message);  // Display the given message and the GetLastError() info

/// Returns the time in seconds with maximum resolution
double time();

class FpsCounter {
  struct Frame {
    double end;
    double cpu_ms;
  };
  std::deque<Frame> frames;
  double frameStart = 0;

 public:
  void startFrame() { frameStart = time(); }
  void endFrame() {
    double now = time();
    // if (frames.size() > 0)
    //   FB2K_console_formatter() << (now - frames.back().end) << ";" << now - frameStart;
    frames.push_back(Frame{now, now - frameStart});
    while (frames.size() > 2 && (now - frames.at(1).end) > 1) frames.pop_front();
  }
  void flush() { frames.clear(); }

  void getFPS(double& avgDur, double& maxDur, double& avgCPU, double& maxCPU) {
    if (frames.size() > 1) {
      double prevEnd = 0;
      for (auto frame : frames) {
        if (prevEnd != 0) {
          double frameDur = frame.end - prevEnd;
          maxDur = std::max(maxDur, frameDur);
        }
        prevEnd = frame.end;
      }
      avgDur = (frames.back().end - frames.front().end) / (frames.size() - 1);
    } else {
      avgDur = std::numeric_limits<double>::quiet_NaN();
      maxDur = std::numeric_limits<double>::quiet_NaN();
    }
    if (frames.size() > 0) {
      maxCPU = std::max_element(frames.begin(), frames.end(),
                                [](Frame a, Frame b) { return a.cpu_ms < b.cpu_ms; })
                   ->cpu_ms;
      avgCPU = std::reduce(frames.begin(), frames.end(), 0.0,
                           [](double a, Frame b) { return a + b.cpu_ms; }) /
               frames.size();
    } else {
      maxCPU = std::numeric_limits<double>::quiet_NaN();
      avgCPU = std::numeric_limits<double>::quiet_NaN();
    }
  }
};

template <typename T>
inline pfc::list_t<T> pfc_list(std::initializer_list<T> elems) {
  pfc::list_t<T> out;
  out.prealloc(elems.size());
  for (auto& e : elems) {
    out.add_item(e);
  }
  return out;
}

class ThreadEntryPoint {
 public:
  [[nodiscard]] ThreadEntryPoint(std::string const& name)
      : stackTracker(("foo_chronflow thread: " + name).c_str()) {
    SetThreadDescription(GetCurrentThread(), pfc::stringcvt::string_wide_from_utf8(
                                                 ("foo_chronflow " + name).c_str()));
  }

 private:
  uCallStackTracker stackTracker;
};

// Wrapper for win32 api calls to raise an exception on failure
template <typename T>
inline T check(T a) {
  if (a != NULL) {
    return a;
  } else {
    WIN32_OP_FAIL();
  }
}

class Timer {
 public:
  Timer(double delay_s, std::function<void()> f) : f(std::move(f)) {
    timer = wil::unique_threadpool_timer{
        check(CreateThreadpoolTimer(Timer::callback, this, nullptr))};
    FILETIME ftime =
        wil::filetime::from_int64(int64_t(-delay_s * wil::filetime_duration::one_second));
    SetThreadpoolTimer(timer.get(), &ftime, 0, 0);
  }

 private:
  static VOID CALLBACK callback(PTP_CALLBACK_INSTANCE /*instance*/, PVOID context,
                                PTP_TIMER /*timer*/) {
    reinterpret_cast<Timer*>(context)->f();
  }

  wil::unique_threadpool_timer timer;
  std::function<void()> f;
};

#ifdef _DEBUG
namespace console {
class out : public std::ostringstream {
 public:
  out();
  NO_MOVE_NO_COPY(out);
  ~out() final;
};
};  // namespace console
#endif

std::string linux_lineendings(std::string s);
std::string windows_lineendings(std::string s);

#ifndef uT
#define uT(x) (pfc::stringcvt::string_os_from_utf8(x).get_ptr())
#define uTS(x, s) (pfc::stringcvt::string_os_from_utf8(x, s).get_ptr())
#define Tu(x) (pfc::stringcvt::string_utf8_from_os(x).get_ptr())
#define TSu(x, s) (pfc::stringcvt::string_utf8_from_os(x, s).get_ptr())
#endif

inline std::wstring wstring_from_utf8(const std::string& s) {
  std::wstring out;
  out.resize(pfc::stringcvt::estimate_utf8_to_wide_quick(s.c_str()));
  auto size =
      pfc::stringcvt::convert_utf8_to_wide(out.data(), out.size(), s.c_str(), s.size());
  out.resize(size);
  return out;
}

template <typename F, typename T, typename U>
decltype(auto) apply_method(F&& func, T&& first, U&& tuple) {
  return std::apply(
      std::forward<F>(func), std::tuple_cat(std::forward_as_tuple(std::forward<T>(first)),
                                            std::forward<U>(tuple)));
}

inline std::function<void(void)> catchThreadExceptions(std::string threadName,
                                                       std::function<void(void)> f) {
  return [=] {
    ThreadEntryPoint thread(threadName);
    try {
      f();
    } catch (exception_aborted&) {
    } catch (std::exception& e) {
      errorPopup(PFC_string_formatter() << threadName.c_str() << " crashed:\n"
                                        << e.what());
    } catch (...) {
      errorPopup(PFC_string_formatter() << threadName.c_str() << " crashed.");
    }
  };
}

template <typename Array, std::size_t... I>
auto array2tuple_impl(const Array& a, std::index_sequence<I...>) {
  return std::make_tuple(a[I]...);
}

template <typename T, std::size_t N, typename Indices = std::make_index_sequence<N>>
auto array2tuple(const std::array<T, N>& a) {
  return array2tuple_impl(a, Indices{});
}

// clang-format off
template <std::size_t I = 0, typename FuncT, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
for_each(std::tuple<Tp...>&, FuncT /* f */) {}

template <std::size_t I = 0, typename FuncT, typename... Tp>
inline typename std::enable_if<I < sizeof...(Tp), void>::type
for_each(std::tuple<Tp...>& t, FuncT f) {
  f(std::get<I>(t));
  for_each<I + 1, FuncT, Tp...>(t, f);
}
// clang-format on
