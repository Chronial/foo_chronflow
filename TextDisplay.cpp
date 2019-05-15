#include "TextDisplay.h"

#include "Renderer.h"
#include "config.h"

BitmapFont::BitmapFont(Renderer& renderer) : renderer(renderer) {
  wil::unique_hfont font{CreateFont(-14,  // Height Of Font
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
                                    L"Courier New")};  // Font Name

  Gdiplus::Bitmap dcBitmap{5, 5, PixelFormat32bppARGB};
  Gdiplus::Graphics dcGraphics{&dcBitmap};

  HDC hdc = dcGraphics.GetHDC();
  // Select The Font We Want
  SelectObject(hdc, font.get());
  glDisplayList = glGenLists(96);
  // Build 96 Characters Starting At Character 32
  wglUseFontBitmaps(hdc, 32, 96, glDisplayList);
  dcGraphics.ReleaseHDC(hdc);
}

BitmapFont::~BitmapFont() {
  glDeleteLists(glDisplayList, 96);
}

void BitmapFont::displayText(const char* text, int x, int y) {
  renderer.glPushOrthoMatrix();
  glDisable(GL_TEXTURE_2D);
  glColor3f(GetRValue(cfgTitleColor) / 255.0f, GetGValue(cfgTitleColor) / 255.0f,
            GetBValue(cfgTitleColor) / 255.0f);
  glRasterPos2i(x, y);
  glPushAttrib(GL_LIST_BIT);
  glListBase(glDisplayList - 32);
  glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
  glPopAttrib();
  glEnable(GL_TEXTURE_2D);
  renderer.glPopOrthoMatrix();
}

TextDisplay::TextDisplay(Renderer& renderer) : renderer(renderer) {
  THROW_IF_FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2Factory));
  THROW_IF_FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                      __uuidof(IDWriteFactory),
                                      reinterpret_cast<IUnknown**>(&writeFactory)));
  THROW_IF_FAILED(writeFactory->GetGdiInterop(&gdiInterop));
  THROW_IF_FAILED(CoCreateInstance(
      CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory)));
}

TextDisplay::~TextDisplay() {
  clearCache();
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

void TextDisplay::displayText(const std::string& text, int x, int y) {
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

  renderer.glPushOrthoMatrix();
  x -= dTex->centerX;
  y += dTex->centerY;

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  glColor3f(1.0f, 1.0f, 1.0f);
  glBindTexture(GL_TEXTURE_2D, dTex->glTex);
  glBegin(GL_QUADS);
  {
    glTexCoord2f(0.0f, 0.0f);  // top left
    glVertex3i(x, y, 0);
    glTexCoord2f(1.0f, 0.0f);  // top right
    glVertex3i(x + dTex->texWidth, y, 0);
    glTexCoord2f(1.0f, 1.0f);  // bottom right
    glVertex3i(x + dTex->texWidth, y - dTex->texHeight, 0);
    glTexCoord2f(0.0f, 1.0f);  // bottom left
    glVertex3i(x, y - dTex->texHeight, 0);
  }
  glEnd();
  glDisable(GL_BLEND);
  renderer.glPopOrthoMatrix();
}

TextDisplay::DisplayTexture TextDisplay::createTexture(const std::string& text) {
  DisplayTexture displayTex;
  displayTex.text = text;
  displayTex.color = cfgTitleColor;

  wil::com_ptr<IDWriteTextFormat> textFormat;
  {  // load font
    wil::com_ptr<IDWriteFont> font{};
    wil::com_ptr<IDWriteFontFamily> fontFamily{};
    wil::com_ptr<IDWriteLocalizedStrings> familyNames{};
    THROW_IF_FAILED(gdiInterop->CreateFontFromLOGFONT(&cfgTitleFont.get_value(), &font));
    THROW_IF_FAILED(font->GetFontFamily(&fontFamily));
    THROW_IF_FAILED(fontFamily->GetFamilyNames(&familyNames));

    std::wstring fontFamilyName{};
    UINT32 fontFamilyLength = 0;
    THROW_IF_FAILED(familyNames->GetStringLength(0, &fontFamilyLength));
    fontFamilyName.resize(fontFamilyLength + 1);
    THROW_IF_FAILED(
        familyNames->GetString(0, fontFamilyName.data(), fontFamilyLength + 1));
    float fontSize = float(abs(cfgTitleFont.get_value().lfHeight));
    THROW_IF_FAILED(writeFactory->CreateTextFormat(
        fontFamilyName.c_str(), NULL, font->GetWeight(), font->GetStyle(),
        font->GetStretch(), fontSize, L"en-us", &textFormat));
  }
  THROW_IF_FAILED(textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER));
  THROW_IF_FAILED(textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR));

  // calculate text size
  pfc::stringcvt::string_wide_from_utf8 w_text(text.c_str());
  wil::com_ptr<IDWriteTextLayout> textLayout;
  THROW_IF_FAILED(writeFactory->CreateTextLayout(
      w_text.get_ptr(), w_text.length(), textFormat.get(), float(renderer.winWidth),
      float(renderer.winHeight), &textLayout));

  DWRITE_TEXT_METRICS textMetrics{};
  THROW_IF_FAILED(textLayout->GetMetrics(&textMetrics));

  // Make the texture size a power of two
  int minTexWidth = int(ceil(textMetrics.width));
  int minTexHeight = int(ceil(textMetrics.height));
  displayTex.centerX = int(ceil(textMetrics.width / 2));
  displayTex.centerY = int(ceil(textMetrics.height / 2));
  auto drawOffset = D2D1::Point2F(-ceil(textMetrics.left), -ceil(textMetrics.top));
  displayTex.texWidth = 1;
  while (displayTex.texWidth < minTexWidth)
    displayTex.texWidth = displayTex.texWidth << 1;
  displayTex.texHeight = 1;
  while (displayTex.texHeight < minTexHeight)
    displayTex.texHeight = displayTex.texHeight << 1;

  wil::com_ptr<IWICBitmap> bitmap;
  THROW_IF_FAILED(wicFactory->CreateBitmap(displayTex.texWidth, displayTex.texHeight,
                                           GUID_WICPixelFormat32bppPRGBA,
                                           WICBitmapCacheOnDemand, &bitmap));
  wil::com_ptr<ID2D1RenderTarget> renderTarget;
  THROW_IF_FAILED(d2Factory->CreateWicBitmapRenderTarget(
      bitmap.get(), D2D1::RenderTargetProperties(), &renderTarget));

  wil::com_ptr<ID2D1SolidColorBrush> textBrush;
  THROW_IF_FAILED(renderTarget->CreateSolidColorBrush(
      D2D1::ColorF(GetRValue(cfgTitleColor) / 255.0f, GetGValue(cfgTitleColor) / 255.0f,
                   GetBValue(cfgTitleColor) / 255.0f, 1.0f),
      &textBrush));

  renderTarget->BeginDraw();
  renderTarget->DrawTextLayout(drawOffset, textLayout.get(), textBrush.get());
  renderTarget->EndDraw();

  wil::com_ptr<IWICBitmapLock> bitmapLock;
  WICRect lockRect{0, 0, displayTex.texWidth, displayTex.texHeight};
  THROW_IF_FAILED(bitmap->Lock(&lockRect, WICBitmapLockRead, &bitmapLock));
  WICInProcPointer bitmapData;
  UINT dataSize;
  THROW_IF_FAILED(bitmapLock->GetDataPointer(&dataSize, &bitmapData));

  glGenTextures(1, &displayTex.glTex);
  glBindTexture(GL_TEXTURE_2D, displayTex.glTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, displayTex.texWidth, displayTex.texHeight, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, bitmapData);

  return displayTex;
}
