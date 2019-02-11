#pragma once

#include "base.h"

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
    int64_t delay_ns = static_cast<int>(-delay_s * 1000 * 1000 * 1000);
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
