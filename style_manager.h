#pragma once
#include "config.h"

class StyleManager {
 public:
  void setChangeHandler(std::function<void()> handler) { changeHandler = handler; };

  COLORREF getTitleColor() {
    return cfgTitleColorCustom ? cfgTitleColor : cachedTitleColor;
  };
  std::array<float, 3> getTitleColorF() {
    return std::array<GLfloat, 3>{GetRValue(getTitleColor()) / 255.0f,
                                  GetGValue(getTitleColor()) / 255.0f,
                                  GetBValue(getTitleColor()) / 255.0f};
  }
  LOGFONT getTitleFont() { return cfgTitleFontCustom ? cfgTitleFont : cachedTitleFont; };

  COLORREF getBgColor() { return cfgPanelBgCustom ? cfgPanelBg : cachedBgColor; };
  std::array<float, 3> getBgColorF() {
    return std::array<GLfloat, 3>{GetRValue(getBgColor()) / 255.0f,
                                  GetGValue(getBgColor()) / 255.0f,
                                  GetBValue(getBgColor()) / 255.0f};
  }

  void onChange() {
    updateCache();
    if (changeHandler)
      changeHandler();
  };

 protected:
  virtual COLORREF defaultTitleColor() = 0;
  virtual LOGFONT defaultTitleFont() = 0;
  virtual COLORREF defaultBgColor() = 0;

  void updateCache() {
    cachedTitleColor = defaultTitleColor();
    cachedTitleFont = defaultTitleFont();
    cachedBgColor = defaultBgColor();
  }

 private:
  std::function<void()> changeHandler;
  COLORREF cachedTitleColor{};
  LOGFONT cachedTitleFont{};
  COLORREF cachedBgColor{};
};
