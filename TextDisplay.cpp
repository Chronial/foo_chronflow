#include "TextDisplay.h"

#include "Renderer.h"
#include "config.h"

TextDisplay::~TextDisplay() {
  clearCache();
  if (bitmapFontInitialized)
    glDeleteLists(bitmapDisplayList, 96);
}

void TextDisplay::buildDisplayFont() {
  HFONT font = CreateFont(-14,  // Height Of Font
                          0,  // Width Of Font
                          0,  // Angle Of Escapement
                          0,  // Orientation Angle
                          FW_NORMAL,  // Font Weight
                          FALSE,  // Italic
                          FALSE,  // Underline
                          FALSE,  // Strikeout
                          ANSI_CHARSET,  // Character Set Identifier
                          OUT_TT_PRECIS,  // Output Precision
                          CLIP_DEFAULT_PRECIS,  // Clipping Precision
                          ANTIALIASED_QUALITY,  // Output Quality
                          FF_SCRIPT | DEFAULT_PITCH,  // Family And Pitch
                          L"Courier New");  // Font Name

  Gdiplus::Bitmap dcBitmap(5, 5, PixelFormat32bppARGB);
  Gdiplus::Graphics dcGraphics(&dcBitmap);

  HDC hDC = dcGraphics.GetHDC();
  // Select The Font We Want
  SelectObject(hDC, font);
  bitmapDisplayList = glGenLists(96);
  // Build 96 Characters Starting At Character 32
  wglUseFontBitmaps(hDC, 32, 96, bitmapDisplayList);
  dcGraphics.ReleaseHDC(hDC);

  DeleteObject(font);
  bitmapFontInitialized = true;
}

void TextDisplay::displayBitmapText(const char* text, int x, int y) {
  if (!bitmapFontInitialized)
    buildDisplayFont();
  renderer->glPushOrthoMatrix();
  glDisable(GL_TEXTURE_2D);
  glColor3f(GetRValue(cfgTitleColor) / 255.0f, GetGValue(cfgTitleColor) / 255.0f,
            GetBValue(cfgTitleColor) / 255.0f);
  glRasterPos2i(x, y);
  glPushAttrib(GL_LIST_BIT);
  glListBase(bitmapDisplayList - 32);
  glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
  glPopAttrib();
  glEnable(GL_TEXTURE_2D);
  renderer->glPopOrthoMatrix();
}

void TextDisplay::clearCache() {
  for (auto& tex : texCache) {
    if (tex.glTex) {
      glDeleteTextures(1, &tex.glTex);
      tex.glTex = 0;
    }
    tex.age = ~0u;
  }
}

void TextDisplay::displayText(const std::string& text, int x, int y, HAlignment hAlign,
                              VAlignment vAlign) {
  DisplayTexture* dTex = nullptr;
  DisplayTexture* oldestElem = &texCache.front();
  unsigned int maxAge = 0;
  for (auto& tex : texCache) {
    if (tex.glTex && tex.text == text && tex.color == cfgTitleColor) {
      dTex = &tex;
      tex.age = 0;
    } else {
      if (tex.age < ~0)
        tex.age++;
      if (tex.age > maxAge) {
        maxAge = tex.age;
        oldestElem = &tex;
      }
    }
  }
  if (dTex == nullptr) {  // not in cache
    if (oldestElem->glTex) {  // is oldest Element initialized?
      glDeleteTextures(1, &oldestElem->glTex);
      oldestElem->glTex = 0;
    }
    *oldestElem = createTexture(text);
    oldestElem->age = 0;
    dTex = oldestElem;
  }

  renderer->glPushOrthoMatrix();
  if (hAlign == right) {
    x -= dTex->textWidth;
  } else if (hAlign == center) {
    x -= dTex->textWidth / 2;
  }

  if (vAlign == bottom) {
    y -= dTex->textHeight;
  } else if (vAlign == middle) {
    y -= dTex->textHeight / 2;
  }

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glColor3f(1.0f, 1.0f, 1.0f);
  glBindTexture(GL_TEXTURE_2D, dTex->glTex);
  glBegin(GL_QUADS);
  {
    glTexCoord2f(0.0f, 1.0f);  // top left
    glVertex3i(x, y + dTex->texHeight, 0);
    glTexCoord2f(1.0f, 1.0f);  // top right
    glVertex3i(x + dTex->texWidth, y + dTex->texHeight, 0);
    glTexCoord2f(1.0f, 0.0f);  // bottom right
    glVertex3i(x + dTex->texWidth, y, 0);
    glTexCoord2f(0.0f, 0.0f);  // bottom left
    glVertex3i(x, y, 0);
  }
  glEnd();
  glDisable(GL_BLEND);
  renderer->glPopOrthoMatrix();
}

TextDisplay::DisplayTexture TextDisplay::createTexture(const std::string& text) {
  DisplayTexture displayTex;
  displayTex.text = text;
  unique_ptr<Gdiplus::Bitmap> bitmap;

  {
    pfc::stringcvt::string_wide_from_utf8 w_text(text.c_str());
    Gdiplus::StringFormat strFormat;
    unique_ptr<Gdiplus::Font> font;
    strFormat.SetAlignment(Gdiplus::StringAlignmentCenter);
    strFormat.SetTrimming(Gdiplus::StringTrimmingNone);
    strFormat.SetFormatFlags(Gdiplus::StringFormatFlagsNoFitBlackBox |
                             Gdiplus::StringFormatFlagsNoWrap |
                             Gdiplus::StringFormatFlagsNoClip);
    Gdiplus::RectF stringSize(0, 0, 1024, 128);

    {  // calculate Text Size
      Gdiplus::Bitmap calcBitmap(5, 5, PixelFormat32bppARGB);
      Gdiplus::Graphics graphics(&calcBitmap);

      HDC fontDC = graphics.GetHDC();
      font = make_unique<Gdiplus::Font>(fontDC, &(cfgTitleFont.get_value()));
      graphics.ReleaseHDC(fontDC);
      if (!font->IsAvailable()) {
        font = make_unique<Gdiplus::Font>(L"Verdana", 8.0f);
      }
      graphics.MeasureString(w_text, -1, font.get(), Gdiplus::PointF(), &stringSize);
    }

    // round to multiples of two, so centering is consistent
    stringSize.Width = ceil(stringSize.Width / 2.0f) * 2;
    stringSize.Height = ceil(stringSize.Height);
    displayTex.texWidth = displayTex.textWidth = static_cast<int>(stringSize.Width);
    displayTex.texHeight = displayTex.textHeight = static_cast<int>(stringSize.Height);

    // Make the texture size a power of two
    displayTex.texWidth = 1;
    while (displayTex.texWidth < displayTex.textWidth)
      displayTex.texWidth = displayTex.texWidth << 1;

    displayTex.texHeight = 1;
    while (displayTex.texHeight < displayTex.textHeight)
      displayTex.texHeight = displayTex.texHeight << 1;

    bitmap = make_unique<Gdiplus::Bitmap>(
        displayTex.texWidth, displayTex.texHeight, PixelFormat32bppARGB);
    Gdiplus::Graphics drawer(bitmap.get());
    drawer.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);

    Gdiplus::Color textColor(255, 255, 255);
    textColor.SetFromCOLORREF(cfgTitleColor);
    Gdiplus::SolidBrush textBrush(textColor);
    displayTex.color = cfgTitleColor;

    drawer.DrawString(w_text, -1, font.get(), stringSize, &strFormat, &textBrush);
  }
  {
    bitmap->RotateFlip(Gdiplus::RotateNoneFlipY);
    Gdiplus::Rect rc(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
    Gdiplus::BitmapData bitmapData{};
    bitmap->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bitmapData);

    glGenTextures(1, &displayTex.glTex);
    glBindTexture(GL_TEXTURE_2D, displayTex.glTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    void* data = bitmapData.Scan0;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, displayTex.texWidth, displayTex.texHeight, 0,
                 GL_BGRA_EXT, GL_UNSIGNED_BYTE, data);

    bitmap->UnlockBits(&bitmapData);
  }
  return displayTex;
}
