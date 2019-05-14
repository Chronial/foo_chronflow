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
  Renderer* renderer;

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

  explicit TextDisplay(Renderer* renderer) : renderer(renderer) {}
  TextDisplay(const TextDisplay&) = delete;
  TextDisplay& operator=(const TextDisplay&) = delete;
  TextDisplay(TextDisplay&&) = delete;
  TextDisplay& operator=(TextDisplay&&) = delete;
  ~TextDisplay();

  void displayText(const std::string& text, int x, int y, HAlignment hAlign,
                   VAlignment vAlign);
  void clearCache();

 private:
  struct DisplayTexture {
    unsigned int age{};
    GLuint glTex = 0;
    std::string text;
    COLORREF color{};
    int textWidth{};
    int textHeight{};
    int texWidth{};
    int texHeight{};
  };

  DisplayTexture createTexture(const std::string& text);
  static const int CACHE_SIZE = 20;
  std::array<DisplayTexture, CACHE_SIZE> texCache;
};
