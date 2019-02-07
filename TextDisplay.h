#pragma once

class Renderer;

class TextDisplay {
  Renderer* renderer;

 public:
  TextDisplay(Renderer* renderer);
  ~TextDisplay();

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

 public:
  void displayText(const char* text, int x, int y, HAlignment hAlign, VAlignment vAlign);
  void clearCache();
  void displayBitmapText(const char* text, int x, int y);

 private:
  GLuint bitmapDisplayList;
  bool bitmapFontInitialized;
  void buildDisplayFont();

 private:
  struct DisplayTexture {
    unsigned int age;
    GLuint glTex = 0;
    std::string text;
    COLORREF color;
    int textWidth;
    int textHeight;
    int texWidth;
    int texHeight;
  };

 private:
  DisplayTexture createTexture(const char* text);
  static const int CACHE_SIZE = 20;
  DisplayTexture texCache[CACHE_SIZE];
};
