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
#define deg2rad(X) ((X)*M_PI / 180)
#define rad2deg(X) ((X)*180 / M_PI)

#define IDT_CHECK_MINIMIZED 665

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

#define TEST_BIT_PTR(BITSET_PTR, BIT) _bittest(BITSET_PTR, BIT)
#ifdef _DEBUG
#define ASSUME(X) PFC_ASSERT(X)
#else
#define ASSUME(X) __assume(X)
#endif

void errorPopup(const char* message);
void errorPopupWin32(
    const char* message);  // Display the given message and the GetLastError() info

class Helpers {
 public:
  // Returns the time in seconds with maximum resolution
  static bool isPerformanceCounterSupported();
  static double getHighresTimer();
  static void fixPath(pfc::string_base& path);
};

class FpsCounter {
  std::array<double, 60> frameTimes{};
  std::array<double, 60> frameDur{};
  int frameTimesP = 0;

 public:
  void recordFrame(double start, double end) {
    double thisFrame = end - start;
    frameTimes.at(frameTimesP) = end;
    frameDur.at(frameTimesP) = thisFrame;
    if (++frameTimesP == 60)
      frameTimesP = 0;
  }

  void getFPS(double& fps, double& msPerFrame, double& longestFrame, double& minFps) {
    double frameDurSum = 0;
    longestFrame = -1;
    double longestFrameTime = -1;
    double thisFrameTime;
    int frameTimesT = frameTimesP - 1;
    if (frameTimesT < 0)
      frameTimesT = 59;

    double lastTime = frameTimes.at(frameTimesT);
    double endTime = lastTime;
    for (int i = 0; i < 30; i++, frameTimesT--) {
      if (frameTimesT < 0)
        frameTimesT = 59;

      frameDurSum += frameDur.at(frameTimesT);
      if (frameDur.at(frameTimesT) > longestFrame)
        longestFrame = frameDur.at(frameTimesT);

      thisFrameTime = lastTime - frameTimes.at(frameTimesT);
      if (thisFrameTime > longestFrameTime)
        longestFrameTime = thisFrameTime;
      lastTime = frameTimes.at(frameTimesT);
    }

    fps = 1 / ((endTime - lastTime) / 29);
    msPerFrame = frameDurSum * 1000 / 30;
    longestFrame *= 1000;
    minFps = 1 / longestFrameTime;
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
    timer = check(CreateThreadpoolTimer(Timer::callback, this, nullptr));
    int64_t delay_ns = static_cast<int>(-delay_s * 1000 * 1000 * 10);
    FILETIME ftime = {static_cast<DWORD>(delay_ns), static_cast<DWORD>(delay_ns >> 32)};
    SetThreadpoolTimer(timer, &ftime, 0, 0);
  }
  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;
  Timer(Timer&&) = delete;
  Timer& operator=(Timer&&) = delete;
  ~Timer() {
    SetThreadpoolTimer(timer, nullptr, 0, 0);
    WaitForThreadpoolTimerCallbacks(timer, 1);
    CloseThreadpoolTimer(timer);
  }

 private:
  static VOID CALLBACK callback(PTP_CALLBACK_INSTANCE /*instance*/, PVOID context,
                                PTP_TIMER /*timer*/) {
    reinterpret_cast<Timer*>(context)->f();
  }

  PTP_TIMER timer;
  std::function<void()> f;
};

#ifdef _DEBUG
namespace console {
class out : public std::wostringstream {
 public:
  NO_MOVE_NO_COPY(out);
  ~out() final;
};
void create();
void print(const wchar_t* str);
void println(const wchar_t* str);
void printf(const wchar_t* format, ...);
};  // namespace console
#endif

std::string linux_lineendings(const std::string& s);
std::string windows_lineendings(const std::string& s);

#ifndef uT
#define uT(x) (pfc::stringcvt::string_os_from_utf8(x).get_ptr())
#define uTS(x, s) (pfc::stringcvt::string_os_from_utf8(x, s).get_ptr())
#define Tu(x) (pfc::stringcvt::string_utf8_from_os(x).get_ptr())
#define TSu(x, s) (pfc::stringcvt::string_utf8_from_os(x, s).get_ptr())
#endif

template <typename F, typename T, typename U>
decltype(auto) apply_method(F&& func, T&& first, U&& tuple) {
  return std::apply(
      std::forward<F>(func), std::tuple_cat(std::forward_as_tuple(std::forward<T>(first)),
                                            std::forward<U>(tuple)));
}

inline std::function<void(void)> catchThreadExceptions(std::string threadName,
                                                       std::function<void(void)> f) {
  return [=] {
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
