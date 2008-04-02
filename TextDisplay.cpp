#include "chronflow.h"

extern cfg_int cfgTitleColor;
extern cfg_struct_t<LOGFONT> cfgTitleFont;

using namespace Gdiplus;


TextDisplay::TextDisplay(AppInstance* appInstance)
: bitmapFontInitialized(false),
  appInstance(appInstance)
{
}

TextDisplay::~TextDisplay(){
	clearCache();
	if (bitmapFontInitialized)
		glDeleteLists(bitmapDisplayList, 96);
}

void TextDisplay::buildDisplayFont(){
	HFONT	font;										// Windows Font ID
	HFONT	oldfont;									// Used For Good House Keeping

	bitmapDisplayList = glGenLists(96);								// Storage For 96 Characters

	font = CreateFont(	-14,							// Height Of Font
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
						FF_DONTCARE|DEFAULT_PITCH,		// Family And Pitch
						L"Courier New");					// Font Name
	HDC hDC = GetDC(appInstance->mainWindow);
	oldfont = (HFONT)SelectObject(hDC, font);           // Selects The Font We Want
	wglUseFontBitmaps(hDC, 32, 96, bitmapDisplayList);				// Builds 96 Characters Starting At Character 32
	SelectObject(hDC, oldfont);							// Selects The Font We Want
	DeleteObject(font);									// Delete The Font
	bitmapFontInitialized = true;
}

void TextDisplay::displayBitmapText(const char* text, int x, int y){
	if (!bitmapFontInitialized)
		buildDisplayFont();
	appInstance->renderer->glPushOrthoMatrix();
	glDisable(GL_TEXTURE_2D);
	glColor3f(GetRValue(cfgTitleColor)/255.0f, GetGValue(cfgTitleColor)/255.0f, GetBValue(cfgTitleColor)/255.0f);
	glRasterPos2i(x, y);
	glPushAttrib(GL_LIST_BIT);							// Pushes The Display List Bits
	glListBase(bitmapDisplayList - 32);								// Sets The Base Character to 32
	glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);	// Draws The Display List Text
	glPopAttrib();								        // Pops The Display List Bits
	glEnable(GL_TEXTURE_2D);
	appInstance->renderer->glPopOrthoMatrix();
}

void TextDisplay::clearCache()
{
	for (int i=0; i < CACHE_SIZE; i++){
		if (texCache[i].text) {
			glDeleteTextures(i, &texCache[i].glTex);
			delete texCache[i].text;
		}
		texCache[i].text = 0;
		texCache[i].age = ~0;
	}
}

void TextDisplay::displayText(const char* text, int x, int y, HAlignment hAlign, VAlignment vAlign)
{
	DisplayTexture* dTex = 0;
	int oldestElem = 0;
	unsigned int maxAge = 0;
	for (int i=0; i < CACHE_SIZE; i++){
		if (texCache[i].text
			&& texCache[i].color == cfgTitleColor
			&& (strcmp(texCache[i].text, text) == 0)){
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
		if (texCache[oldestElem].text) { // is oldest Element initialized?
			glDeleteTextures(1, &texCache[oldestElem].glTex);
			delete texCache[oldestElem].text;
		}
		texCache[oldestElem] = createTexture(text);
		texCache[oldestElem].age = 0;
		dTex = &texCache[oldestElem];
	}

	appInstance->renderer->glPushOrthoMatrix();
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
	appInstance->renderer->glPopOrthoMatrix();
}

TextDisplay::DisplayTexture TextDisplay::createTexture(const char* text)
{
	DisplayTexture displayTex;
	size_t textLen = strlen(text);
	displayTex.text = new char[textLen+1];
	strcpy_s(displayTex.text, textLen+1, text);
	Bitmap* bitmap;
	Gdiplus::Font* font;

	{
		pfc::stringcvt::string_wide_from_utf8 w_text(text);
		StringFormat strFormat;
		strFormat.SetAlignment(StringAlignmentCenter);
		strFormat.SetTrimming(StringTrimmingNone);
		strFormat.SetFormatFlags(StringFormatFlagsNoFitBlackBox |
								 StringFormatFlagsMeasureTrailingSpaces | 
								 StringFormatFlagsNoWrap |
								 StringFormatFlagsNoClip);
		RectF stringSize(0.0, 0.0, 0.0, 0.0);

		{ // calculate Text Size
			Bitmap calcBitmap(5, 5, PixelFormat32bppARGB);
			Graphics graphics(&calcBitmap);
			CharacterRange charRange(0, wcslen(w_text));

			strFormat.SetMeasurableCharacterRanges(1, &charRange);
			Region charRangeRegion;
			
			{
				HDC fontDC = graphics.GetHDC();
				font = new Gdiplus::Font(fontDC, &(cfgTitleFont.get_value()));
				graphics.ReleaseHDC(fontDC);
				if (!font->IsAvailable()){
					delete font;
					font = new Gdiplus::Font(L"Verdana", 8.0f);
				}
			}


			Status res = graphics.MeasureCharacterRanges(w_text, -1,
				font, RectF(0.0f, 0.0f, 1024.0f, 128.0f), &strFormat, 1, &charRangeRegion);
			if (res == Ok){
				charRangeRegion.GetBounds(&stringSize,&graphics);
			} else {
				stringSize.Width = stringSize.Height = stringSize.X = stringSize.Y = 10;
			}
		}
		stringSize.Height += stringSize.Y * 2 + 2;
		stringSize.Width += 10 + stringSize.Width / 10;
		stringSize.X = stringSize.Y = 0;
		displayTex.texWidth  = displayTex.textWidth  = (int)ceil(stringSize.Width);
		displayTex.texHeight = displayTex.textHeight = (int)ceil(stringSize.Height);
		if (ImgTexture::forcePowerOfTwo){
			displayTex.texWidth = 1;
			while (displayTex.texWidth < displayTex.textWidth)
				displayTex.texWidth = displayTex.texWidth << 1;

			displayTex.texHeight = 1;
			while (displayTex.texHeight < displayTex.textHeight)
				displayTex.texHeight = displayTex.texHeight << 1;
		}

		bitmap = new Bitmap(displayTex.texWidth, displayTex.texHeight, PixelFormat32bppARGB);
		Graphics drawer(bitmap);
		drawer.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);

		Color textColor(255, 255, 255);
		textColor.SetFromCOLORREF(cfgTitleColor);
		SolidBrush textBrush(textColor);
		displayTex.color = cfgTitleColor;

		drawer.DrawString(w_text, -1, font, PointF(stringSize.Width/2,0), &strFormat, &textBrush);
		drawer.DrawString(w_text, -1, font, stringSize, &strFormat, &textBrush);
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
	delete bitmap;
	delete font;

	return displayTex;
}