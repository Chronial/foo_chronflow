#pragma once

#include "Image.h"
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

  void displayText(const std::string& text, int x, int y);
  void clearCache();

 private:
  struct DisplayTexture {
    unsigned int age = 0;
    GLTexture glTex;
    std::string text;
    COLORREF color{};
    int texWidth = 0;
    int texHeight = 0;
    int centerX = 0;
    int centerY = 0;
  };

  const DisplayTexture& getTexture(const std::string& text);
  DisplayTexture createTexture(const std::string& text);
  static const int cache_size = 10;
  std::vector<DisplayTexture> texCache;
};
