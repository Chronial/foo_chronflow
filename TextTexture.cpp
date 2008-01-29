#include "chronflow.h"
/*
using namespace Gdiplus;

TextTexture::TextTexture(wchar_t* text)
{
	generateBitmap(text);
}

TextTexture::~TextTexture(void)
{
}


void TextTexture::generateBitmap(wchar_t* text)
{
	Font         myFont(L"Verdana", 16.0f);
	StringFormat strFormat;
	strFormat.SetTrimming(StringTrimmingNone);
	strFormat.SetFormatFlags(StringFormatFlagsNoFitBlackBox |
							 StringFormatFlagsMeasureTrailingSpaces | 
							 StringFormatFlagsNoWrap |
							 StringFormatFlagsNoClip);
	RectF stringSize(0.0, 0.0, 0.0, 0.0);

	{ // calculate Text Size
		static Bitmap calcBitmap(5, 5, PixelFormat32bppARGB);
		static Graphics graphics(&calcBitmap);
		CharacterRange charRange(0, wcslen(text));

		strFormat.SetMeasurableCharacterRanges(1, &charRange);
		Region charRangeRegion;

		graphics.MeasureCharacterRanges(text, -1,
			&myFont, RectF(0.0f, 0.0f, 1024.0f, 128.0f), &strFormat, 1, &charRangeRegion);
		
		charRangeRegion.GetBounds(&stringSize,&graphics);
	}
	SolidBrush whiteBrush(Color(255, 255, 255));
	SolidBrush blackBrush(Color(255, 0, 0));
	stringSize.Width += stringSize.X * 2;
	stringSize.Height += stringSize.Y * 2;
	stringSize.X = stringSize.Y = 0;
	bitmap = new Bitmap((int)ceil(stringSize.Width), (int)ceil(stringSize.Height), PixelFormat32bppARGB);
	Graphics drawer(bitmap);
	drawer.DrawString(text, -1, &myFont, stringSize, &strFormat, &whiteBrush);
	prepareUpload();
}
*/