#include "TextDisplay.h"

#include "Renderer.h"
#include "style_manager.h"

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

void BitmapFont::displayText(const char* text, COLORREF color, int x, int y) {
  renderer.glPushOrthoMatrix();
  glDisable(GL_TEXTURE_2D);
  glColor3f(
      GetRValue(color) / 255.0f, GetGValue(color) / 255.0f, GetBValue(color) / 255.0f);
  glRasterPos2i(x, y);
  glPushAttrib(GL_LIST_BIT);
  glListBase(glDisplayList - 32);
  glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
  glPopAttrib();
  glEnable(GL_TEXTURE_2D);
  renderer.glPopOrthoMatrix();
}

TextDisplay::TextDisplay(Renderer& renderer, StyleManager& styleManager)
    : renderer(renderer), styleManager(styleManager) {
  THROW_IF_FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2Factory));
  THROW_IF_FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                      __uuidof(IDWriteFactory),
                                      reinterpret_cast<IUnknown**>(&writeFactory)));
  THROW_IF_FAILED(writeFactory->GetGdiInterop(&gdiInterop));
  THROW_IF_FAILED(CoCreateInstance(
      CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory)));
}

void TextDisplay::clearCache() {
  texCache.clear();
}

const TextDisplay::DisplayTexture& TextDisplay::getTexture(const std::string& text,
                                                           int highlight) {
  auto cached =
      std::find_if(texCache.begin(), texCache.end(), [&](const DisplayTexture& e) {
        return e.text == text && e.highlight == highlight;
      });
  if (cached != texCache.end()) {
    cached->age = 0;
    return *cached;
  }
  if (texCache.size() >= cache_size) {
    auto oldest = std::max_element(
        texCache.begin(), texCache.end(),
        [](const DisplayTexture& a, const DisplayTexture& b) { return a.age < b.age; });
    texCache.erase(oldest);
  }
  texCache.push_back(createTexture(text, highlight));
  return texCache.back();
}

void TextDisplay::displayText(const std::string& text, int highlight, int x, int y) {
  const DisplayTexture& texture = getTexture(text, highlight);
  renderer.glPushOrthoMatrix();
  x -= texture.centerX;
  y += texture.centerY;

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  glColor3f(1.0f, 1.0f, 1.0f);
  texture.glTex.bind();
  glBegin(GL_QUADS);
  {
    glTexCoord2f(0.0f, 0.0f);  // top left
    glVertex3i(x, y, 0);
    glTexCoord2f(1.0f, 0.0f);  // top right
    glVertex3i(x + texture.texWidth, y, 0);
    glTexCoord2f(1.0f, 1.0f);  // bottom right
    glVertex3i(x + texture.texWidth, y - texture.texHeight, 0);
    glTexCoord2f(0.0f, 1.0f);  // bottom left
    glVertex3i(x, y - texture.texHeight, 0);
  }
  glEnd();
  glDisable(GL_BLEND);
  renderer.glPopOrthoMatrix();
}

TextDisplay::DisplayTexture TextDisplay::createTexture(const std::string& text,
                                                       int highlight) {
  DisplayTexture displayTex;
  displayTex.text = text;
  displayTex.highlight = highlight;

  wil::com_ptr<IDWriteTextFormat> textFormat;
  {  // load font
    wil::com_ptr<IDWriteFont> font{};
    wil::com_ptr<IDWriteFontFamily> fontFamily{};
    wil::com_ptr<IDWriteLocalizedStrings> familyNames{};
    LOGFONT logFont = styleManager.getTitleFont();
    THROW_IF_FAILED(gdiInterop->CreateFontFromLOGFONT(&logFont, &font));
    THROW_IF_FAILED(font->GetFontFamily(&fontFamily));
    THROW_IF_FAILED(fontFamily->GetFamilyNames(&familyNames));

    std::wstring fontFamilyName{};
    UINT32 fontFamilyLength = 0;
    THROW_IF_FAILED(familyNames->GetStringLength(0, &fontFamilyLength));
    fontFamilyName.resize(fontFamilyLength + 1);
    THROW_IF_FAILED(
        familyNames->GetString(0, fontFamilyName.data(), fontFamilyLength + 1));
    float fontSize = float(abs(logFont.lfHeight));
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
  wil::com_ptr<ID2D1RenderTarget> renderTarget;
  try {
    // Use BGR for Vista support
    THROW_IF_FAILED(wicFactory->CreateBitmap(displayTex.texWidth, displayTex.texHeight,
                                             GUID_WICPixelFormat32bppPBGRA,
                                             WICBitmapCacheOnDemand, &bitmap));
    THROW_IF_FAILED(d2Factory->CreateWicBitmapRenderTarget(
        bitmap.get(), D2D1::RenderTargetProperties(), &renderTarget));
  } catch (const wil::ResultException& e) {
    throw std::runtime_error(
        PFC_string_formatter()
        << "Failed to create WIC bitmap or render target for DirectWrite:\n"
        << std::system_category().message(e.GetErrorCode()).c_str() << " [0x"
        << pfc::format_hex((unsigned int)e.GetErrorCode()) << "]\n\n"
        << "If you are running Windows Vista, please install the \"Platform Update "
           "for Windows Vista\".");
  }

  wil::com_ptr<ID2D1SolidColorBrush> textBrush;
  auto color = styleManager.getTitleColorF();
  // Swap Red and Blue so we can treat this BGR image as RGB
  THROW_IF_FAILED(renderTarget->CreateSolidColorBrush(
      D2D1::ColorF(color[2], color[1], color[0], 1.0f), &textBrush));
  if (highlight > 0) {
    wil::com_ptr<ID2D1SolidColorBrush> highlightBrush;
    THROW_IF_FAILED(
        renderTarget->CreateSolidColorBrush(textBrush->GetColor(), &highlightBrush));
    textLayout->SetDrawingEffect(
        highlightBrush.get(), DWRITE_TEXT_RANGE{0, UINT32(highlight)});
    textBrush->SetOpacity(0.4f);
  }

  renderTarget->BeginDraw();
  renderTarget->DrawTextLayout(drawOffset, textLayout.get(), textBrush.get());
  renderTarget->EndDraw();

  wil::com_ptr<IWICBitmapLock> bitmapLock;
  WICRect lockRect{0, 0, displayTex.texWidth, displayTex.texHeight};
  THROW_IF_FAILED(bitmap->Lock(&lockRect, WICBitmapLockRead, &bitmapLock));
  WICInProcPointer bitmapData;
  UINT dataSize;
  THROW_IF_FAILED(bitmapLock->GetDataPointer(&dataSize, &bitmapData));

  displayTex.glTex.bind();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, displayTex.texWidth, displayTex.texHeight, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, bitmapData);

  return displayTex;
}
