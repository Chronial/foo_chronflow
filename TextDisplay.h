#pragma once
#include "Renderer.fwd.h"
#include "style_manager.h"
#include "Image.h"
#include "utils.h"

namespace render {

class BitmapFont {
 public:
  BitmapFont(Renderer& renderer);
  ~BitmapFont();
  NO_MOVE_NO_COPY(BitmapFont);
  void displayText(const char* text, COLORREF color, int x, int y);

 private:
  Renderer& renderer;
  GLuint glDisplayList = 0;
};

class TextDisplay {
  Renderer& renderer;
  StyleManager& styleManager;
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

  explicit TextDisplay(Renderer& renderer, StyleManager& styleManager);

  void displayText(const std::string& text, std::vector<size_t> highlight, int x, int y);
  void clearCache();

 private:
  struct DisplayTexture {
    std::string text;
    std::vector<size_t> highlight;

    unsigned int age = 0;

    GLTexture glTex;
    int texWidth = 0;
    int texHeight = 0;
    int centerX = 0;
    int centerY = 0;
  };

  const DisplayTexture& getTexture(const std::string& text,
                                   const std::vector<size_t>& highlight);
  DisplayTexture createTexture(const std::string& text,
                               const std::vector<size_t>& highlight);
  static const int cache_size = 10;
  std::vector<DisplayTexture> texCache;
};
}  // render namespace
