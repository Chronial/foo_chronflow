#include "chronflow.h"

using namespace Gdiplus;

ImgFileTexture::ImgFileTexture(const wchar_t* imageFile)
{
	size_t len = wcslen(imageFile);
	wcscpy_s(this->imageFile,imageFile);
	loadImage();
}

ImgFileTexture::~ImgFileTexture(void)
{
}

int ImgFileTexture::getMaxSize(){
	return min(512, maxGlTextureSize);
}

void ImgFileTexture::loadImage()
{
	bitmap = new Bitmap(imageFile);
	if ((bitmap->GetLastStatus() != Ok) ||
		(1 > (bitmap->GetWidth())) ||
		(1 > (bitmap->GetHeight()))){
		delete bitmap;
		bitmap = getErrorBitmap();
	}
	prepareUpload();
}
