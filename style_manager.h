#pragma once
namespace render {

class StyleManager {
 public:
  COLORREF getTitleColor();
  LOGFONT getTitleFont();
  COLORREF getBgColor();

  void setChangeHandler(std::function<void()> handler) { changeHandler = handler; };

  std::array<float, 3> getTitleColorF() {
    return std::array<GLfloat, 3>{GetRValue(getTitleColor()) / 255.0f,
                                  GetGValue(getTitleColor()) / 255.0f,
                                  GetBValue(getTitleColor()) / 255.0f};
  }

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
} // namespace
