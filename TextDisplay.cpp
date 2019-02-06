#include "stdafx.h"
#include "config.h"

#include "TextDisplay.h"

#include "Renderer.h"
using namespace Gdiplus;


TextDisplay::TextDisplay(Renderer* renderer)
: bitmapFontInitialized(false),
  renderer(renderer)
{
}

TextDisplay::~TextDisplay(){
	clearCache();
	if (bitmapFontInitialized)
		glDeleteLists(bitmapDisplayList, 96);
}

void TextDisplay::buildDisplayFont(){
	HFONT font = CreateFont(	-14,							// Height Of Font
						0,								// Width Of Font
						0,								// Angle Of Escapement
						0,								// Orientation Angle
						FW_NORMAL,						// Font Weight
						FALSE,							// Italic
						FALSE,							// Underline
						FALSE,							// Strikeout
						ANSI_CHARSET,					// Character Set Identifier
						OUT_TT_PRECIS,					// Output Precision
						CLIP_DEFAULT_PRECIS,			// Clipping Precision
						ANTIALIASED_QUALITY,			// Output Quality
						FF_SCRIPT|DEFAULT_PITCH,		// Family And Pitch
						L"Courier New");					// Font Name

	Gdiplus::Bitmap dcBitmap(5, 5, PixelFormat32bppARGB);
	Gdiplus::Graphics dcGraphics(&dcBitmap);

	HDC hDC = dcGraphics.GetHDC();
	SelectObject(hDC, font);           // Selects The Font We Want
	bitmapDisplayList = glGenLists(96);
	wglUseFontBitmaps(hDC, 32, 96, bitmapDisplayList);				// Builds 96 Characters Starting At Character 32
	dcGraphics.ReleaseHDC(hDC);

	DeleteObject(font);									// Delete The Font
	bitmapFontInitialized = true;
}

void TextDisplay::displayBitmapText(const char* text, int x, int y){
	if (!bitmapFontInitialized)
		buildDisplayFont();
	renderer->glPushOrthoMatrix();
	glDisable(GL_TEXTURE_2D);
	glColor3f(GetRValue(cfgTitleColor)/255.0f, GetGValue(cfgTitleColor)/255.0f, GetBValue(cfgTitleColor)/255.0f);
	glRasterPos2i(x, y);
	glPushAttrib(GL_LIST_BIT);							// Pushes The Display List Bits
	glListBase(bitmapDisplayList - 32);								// Sets The Base Character to 32
	glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);	// Draws The Display List Text
	glPopAttrib();								        // Pops The Display List Bits
	glEnable(GL_TEXTURE_2D);
	renderer->glPopOrthoMatrix();
}

void TextDisplay::clearCache()
{
	for (int i=0; i < CACHE_SIZE; i++){
		if (texCache[i].glTex) {
			glDeleteTextures(i, &texCache[i].glTex);
		}
		texCache[i].age = ~0u;
	}
}

void TextDisplay::displayText(const char* text, int x, int y, HAlignment hAlign, VAlignment vAlign)
{
	if (text == 0 || text[0] == '\0')
		return;
	
	DisplayTexture* dTex = 0;
	int oldestElem = 0;
	unsigned int maxAge = 0;
	for (int i=0; i < CACHE_SIZE; i++){
		if (texCache[i].glTex &&
				texCache[i].text == text &&
				texCache[i].color == cfgTitleColor){
			dTex = &texCache[i];
			texCache[i].age = 0;
		} else {
			if (texCache[i].age < ~0)
				texCache[i].age++;
			if (texCache[i].age > maxAge){
				maxAge = texCache[i].age;
				oldestElem = i;
			}
		}
	}
	if (!dTex) { // not in cache
		if (texCache[oldestElem].glTex) { // is oldest Element initialized?
			glDeleteTextures(1, &texCache[oldestElem].glTex);
			texCache[oldestElem].glTex = 0;
		}
		texCache[oldestElem] = createTexture(text);
		texCache[oldestElem].age = 0;
		dTex = &texCache[oldestElem];
	}

	renderer->glPushOrthoMatrix();
	if (hAlign == right)
		x -= dTex->textWidth;
	else if (hAlign == center)
		x -= dTex->textWidth/2;

	if (vAlign == bottom)
		y -= dTex->textHeight;
	else if (vAlign == middle)
		y -= dTex->textHeight/2;

	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor3f( 1.0f, 1.0f, 1.0f);
	glBindTexture(GL_TEXTURE_2D, dTex->glTex);
	glBegin(GL_QUADS);
	{
		glTexCoord2f(0.0f, 1.0f); // top left
		glVertex3i(x, y+dTex->texHeight, 0);
		glTexCoord2f(1.0f, 1.0f); // top right
		glVertex3i(x+dTex->texWidth, y+dTex->texHeight, 0);
		glTexCoord2f(1.0f, 0.0f); // bottom right
		glVertex3i(x+dTex->texWidth, y, 0);
		glTexCoord2f(0.0f, 0.0f); // bottom left
		glVertex3i(x, y, 0);
	}
	glEnd();
	glDisable(GL_BLEND);
	renderer->glPopOrthoMatrix();
}

TextDisplay::DisplayTexture TextDisplay::createTexture(const char* text)
{
	DisplayTexture displayTex;
	displayTex.text = std::string(text);
	unique_ptr<Bitmap> bitmap;

	{
		pfc::stringcvt::string_wide_from_utf8 w_text(text);
		StringFormat strFormat;
		unique_ptr<Gdiplus::Font> font;
		strFormat.SetAlignment(StringAlignmentCenter);
		strFormat.SetTrimming(StringTrimmingNone);
		strFormat.SetFormatFlags(StringFormatFlagsNoFitBlackBox |
								 StringFormatFlagsNoWrap |
								 StringFormatFlagsNoClip);
		RectF stringSize(0, 0, 1024, 128);

		{ // calculate Text Size
			Bitmap calcBitmap(5, 5, PixelFormat32bppARGB);
			Graphics graphics(&calcBitmap);

			HDC fontDC = graphics.GetHDC();
			font = make_unique<Gdiplus::Font>(fontDC, &(cfgTitleFont.get_value()));
			graphics.ReleaseHDC(fontDC);
			if (!font->IsAvailable()){
				font = make_unique<Gdiplus::Font>(L"Verdana", 8.0f);
			}
			graphics.MeasureString(w_text, -1, font.get(), PointF(), &stringSize);
		}

		// round to multiples of two, so centering is consistent
		stringSize.Width = ceil(stringSize.Width / 2.0f) * 2;
		stringSize.Height = ceil(stringSize.Height);
		displayTex.texWidth  = displayTex.textWidth  = (int)stringSize.Width;
		displayTex.texHeight = displayTex.textHeight = (int)stringSize.Height;
		
		// Make the texture size a power of two
		displayTex.texWidth = 1;
		while (displayTex.texWidth < displayTex.textWidth)
			displayTex.texWidth = displayTex.texWidth << 1;

		displayTex.texHeight = 1;
		while (displayTex.texHeight < displayTex.textHeight)
			displayTex.texHeight = displayTex.texHeight << 1;

		bitmap = make_unique<Bitmap>(displayTex.texWidth, displayTex.texHeight, PixelFormat32bppARGB);
		Graphics drawer(bitmap.get());
		drawer.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);

		Color textColor(255, 255, 255);
		textColor.SetFromCOLORREF(cfgTitleColor);
		SolidBrush textBrush(textColor);
		displayTex.color = cfgTitleColor;

		drawer.DrawString(w_text, -1, font.get(), stringSize, &strFormat, &textBrush);
	}
	{
		bitmap->RotateFlip(RotateNoneFlipY);
		Rect rc(0,0,bitmap->GetWidth(),bitmap->GetHeight());
		BitmapData bitmapData;
		bitmap->LockBits(&rc,ImageLockModeRead,PixelFormat32bppARGB,&bitmapData);

		glGenTextures(1,&displayTex.glTex);
		glBindTexture(GL_TEXTURE_2D, displayTex.glTex);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);

		void* data = bitmapData.Scan0;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, displayTex.texWidth, displayTex.texHeight, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE,data);

		bitmap->UnlockBits(&bitmapData);

	}
	return displayTex;
}