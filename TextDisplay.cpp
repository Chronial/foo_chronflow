#include "chronflow.h"

extern cfg_int cfgTitleColor;
extern cfg_struct_t<LOGFONT> cfgTitleFont;

using namespace Gdiplus;


TextDisplay::TextDisplay(AppInstance* appInstance)
{
	this->appInstance = appInstance;
}

TextDisplay::~TextDisplay(){
	clearCache();
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
	glPushMatrix();
	glLoadIdentity();
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
	glPopMatrix();
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


			graphics.MeasureCharacterRanges(w_text, -1,
				font, RectF(0.0f, 0.0f, 1024.0f, 128.0f), &strFormat, 1, &charRangeRegion);
			
			charRangeRegion.GetBounds(&stringSize,&graphics);
		}
		//stringSize.Width += stringSize.X * 2;
		stringSize.Height += stringSize.Y * 2;
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

		Color textColor(255, 255, 255);
		textColor.SetFromCOLORREF(cfgTitleColor);
		SolidBrush textBrush(textColor);
		displayTex.color = cfgTitleColor;

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