#pragma once

#include "utils.h"

class Renderer;

class BitmapFont {
 public:
  BitmapFont(Renderer& renderer);
  ~BitmapFont();
  NO_MOVE_NO_COPY(BitmapFont);
  void displayText(const char* text, int x, int y);

 private:
  Renderer& renderer;
  GLuint glDisplayList = 0;
};

class TextDisplay {
  Renderer& renderer;
  wil::com_ptr<IDWriteFactory> writeFactory;
  wil::com_ptr<IDWriteGdiInterop> gdiInterop;
  wil::com_ptr<ID2D1Factory> d2Factory;
  wil::com_ptr<IWICImagingFactory> wicFactory;

 public:
  enum HAlignment {
    left,
    center,
    right,
  };
  enum VAlignment {
    top,
    middle,
    bottom,
  };

  explicit TextDisplay(Renderer& renderer);
  TextDisplay(const TextDisplay&) = delete;
  TextDisplay& operator=(const TextDisplay&) = delete;
  TextDisplay(TextDisplay&&) = delete;
  TextDisplay& operator=(TextDisplay&&) = delete;
  ~TextDisplay();

  void displayText(const std::string& text, int x, int y);
  void clearCache();

 private:
  struct DisplayTexture {
    unsigned int age{};
    GLuint glTex = 0;
    std::string text;
    COLORREF color{};
    int texWidth{};
    int texHeight{};
    int centerX = 0;
    int centerY = 0;
  };

  DisplayTexture createTexture(const std::string& text);
  static const int CACHE_SIZE = 20;
  std::array<DisplayTexture, CACHE_SIZE> texCache;
};
