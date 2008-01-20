#include "chronflow.h"
#include "ImgTexture.h"

using namespace Gdiplus;

bool ImgTexture::forcePowerOfTwo = false;
int  ImgTexture::maxGlTextureSize = 128;

ImgTexture::ImgTexture()
{
	status = STATUS_NONE;
	bitmap = 0;
	bitmapData = 0;
	glTexture = 0;
}

ImgTexture::~ImgTexture(void)
{
	if (bitmap)
		delete bitmap;
	if (bitmapData)
		delete bitmapData;
	if (glTexture){
		MessageBoxW(NULL,L"Destructed ImgTexture with existing glTexture\nMemory Leak!",L"Internal Error",MB_OK |MB_ICONINFORMATION);
	}
}

void ImgTexture::glBind(void)
{
	switch (status){
		case STATUS_NONE:
			return;
		case STATUS_IMG_LOCKED:
#ifdef _DEBUG
			Console::println(L"                              forced");
#endif
			glUpload();
	}
	glBindTexture(GL_TEXTURE_2D,glTexture[0]);
}

Bitmap* ImgTexture::getErrorBitmap(){
	Bitmap* bitmap = new Bitmap(256,256);
	Graphics drawer(bitmap);
	SolidBrush blackBrush(Color(0,0,0));
	Pen redPen(Color(255, 0, 0), 10);
	Rect imgRect (0,0,256,256);
	drawer.FillRectangle(&blackBrush,imgRect);
	drawer.DrawLine(&redPen,40,40,256-40,256-40);
	drawer.DrawLine(&redPen,40,256-40,256-40,40);
	
	SolidBrush whiteBrush(Color(255,255,255));
	Font font(L"Verdana",15.0);
	StringFormat format;
	format.SetAlignment(StringAlignmentCenter);
	format.SetLineAlignment(StringAlignmentCenter);
	drawer.DrawString(L"Couldn't load Image",-1,&font,RectF(0,0,256,256),&format,&whiteBrush);
	return bitmap;
}

void ImgTexture::glUpload(void)
{
	if ((status == STATUS_IMG_LOCKED) && bitmap && bitmapData){
#ifdef _DEBUG
			Console::println(L"                                   Upload");
#endif
		glTexture = new GLuint[1];
		glGenTextures(1,glTexture);
		glBindTexture(GL_TEXTURE_2D,glTexture[0]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

		int width = bitmap->GetWidth();
		int height = bitmap->GetHeight();
		void* data = bitmapData->Scan0;

		GLint glInternalFormat;
		PixelFormat pxFmt = bitmap->GetPixelFormat();

		if (IsAlphaPixelFormat(pxFmt) || IsIndexedPixelFormat(pxFmt))
			glInternalFormat = GL_COMPRESSED_RGBA;
		else
			glInternalFormat = GL_COMPRESSED_RGB;
		glTexImage2D(GL_TEXTURE_2D,0,glInternalFormat,width,height,0,GL_BGRA_EXT, GL_UNSIGNED_BYTE,data);
		bitmap->UnlockBits(bitmapData);
		delete bitmapData;
		bitmapData = 0;
		delete bitmap;
		bitmap = 0;
		status = STATUS_UPLOADED;
	}
}

void ImgTexture::glDelete(void)
{
	if (glTexture){
		glDeleteTextures(1,glTexture);
		delete[] glTexture;
	}
	glTexture = 0;
}

float ImgTexture::getAspect()
{
	return aspect;
}

void ImgTexture::setForcePowerOfTwo(bool force)
{
	forcePowerOfTwo = force;
}

void ImgTexture::setMaxGlTextureSize(int size)
{
	maxGlTextureSize = size;
}


int ImgTexture::getMaxSize(){
	return maxGlTextureSize;
}

void ImgTexture::prepareUpload(void)
{
	int maxSize = getMaxSize();
	int width = bitmap->GetWidth();;
	int height = bitmap->GetHeight();;

	bitmap->RotateFlip(RotateNoneFlipY);

	aspect = float(width)/height;
	bool resize = false;

	if ((width > maxSize) || (height > maxSize)){
		if (width > height){
			height = int(maxSize / aspect);
			width  = maxSize;
		} else {
			width  = int(aspect * maxSize);
			height = maxSize;
		}
		resize = true;
	}
	if (forcePowerOfTwo){
		int p2w = 1;
		int p2h = 1;
		while (p2w < width)
			p2w = p2w << 1;
		while (p2h < height)
			p2h = p2h << 1;
		if (p2h != height || p2w != width){
			height = p2h;
			width = p2w;
			resize = true;
		}
	}
	if (resize){
		Bitmap* oldBitmap = bitmap;
		bitmap = new Bitmap(width,height);
		Graphics resizer(bitmap);
		//resizer.SetInterpolationMode(InterpolationModeHighQualityBicubic);
		resizer.DrawImage(oldBitmap,0,0,width,height);
		resizer.Flush();
		delete oldBitmap;
	}
	Rect rc(0,0,bitmap->GetWidth(),bitmap->GetHeight());
	bitmapData = new BitmapData();
	bitmap->LockBits(&rc,ImageLockModeRead,PixelFormat32bppARGB,bitmapData);
	status = STATUS_IMG_LOCKED;
}
