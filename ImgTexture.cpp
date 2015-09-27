#include "stdafx.h"
#include "base.h"
#include "config.h"

#include "ImgTexture.h"

#include "Console.h"

#include "Helpers.h"
#include "CGdiPlusBitmap.h"

using namespace Gdiplus;

int  ImgTexture::maxGlTextureSize = 512;

ImgTexture::ImgTexture()
{
	InitializeCriticalSectionAndSpinCount(&uploadCS, 0x80000400);
}


ImgTexture::ImgTexture(const char * imageFile) : ImgTexture()
{
	this->name = imageFile;
	loadImageFile(imageFile);
}

ImgTexture::ImgTexture(WORD resource, LPCTSTR type) : ImgTexture()
{
	this->name = "internal image";
	loadImageResource(resource, type);
}

ImgTexture::ImgTexture(const album_art_data::ptr &art) : ImgTexture()
{
	this->name = "Album Art";
	loadImageFromArt(art);
}

ImgTexture::~ImgTexture(void)
{
	if (glTexture){
		IF_DEBUG(__debugbreak());
	}
	DeleteCriticalSection(&uploadCS);
}

void ImgTexture::glBind(void)
{
	switch (status){
		case STATUS_NONE:
			IF_DEBUG(__debugbreak());
			return;
		case STATUS_IMG_LOCKED:
			IF_DEBUG(Console::println(L"                                     forced"));
			glUpload();
	}
	glBindTexture(GL_TEXTURE_2D,glTexture[0]);
}

unique_ptr<Bitmap> ImgTexture::getErrorBitmap(){
	unique_ptr<Bitmap> bitmap = make_unique<Bitmap>(256, 256);
	Graphics drawer(bitmap.get());
	SolidBrush blackBrush(Color(0,0,0));
	Pen redPen(Color(255, 0, 0), 10);
	Rect imgRect (0,0,256,256);
	drawer.FillRectangle(&blackBrush,imgRect);
	drawer.DrawLine(&redPen,40,40,256-40,256-40);
	drawer.DrawLine(&redPen,40,256-40,256-40,40);
	
	SolidBrush whiteBrush(Color(255,255,255));
	Gdiplus::Font font(L"Verdana",15.0);
	StringFormat format;
	format.SetAlignment(StringAlignmentCenter);
	format.SetLineAlignment(StringAlignmentCenter);
	drawer.DrawString(pfc::stringcvt::string_wide_from_utf8(name),-1,&font,RectF(0,0,256,256),&format,&whiteBrush);
	return bitmap;
}

void ImgTexture::glUpload(void)
{
	TRACK_CALL_TEXT("ImgTexture::glUpload");
	IF_DEBUG(profiler(ImgTexture__glUpload));
	EnterCriticalSection(&uploadCS);
	if ((status == STATUS_IMG_LOCKED) && bitmap && bitmapData){
		glTexture = new GLuint[1];
		glGenTextures(1,glTexture);
		glBindTexture(GL_TEXTURE_2D,glTexture[0]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

		/*glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_GENERATE_MIPMAP,GL_TRUE);*/


		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);

		int width = bitmap->GetWidth();
		int height = bitmap->GetHeight();
		void* data = bitmapData->Scan0;

#ifdef COVER_ALPHA
		GLint glInternalFormat;
		if (bitmapDataFormat == GL_BGRA_EXT){
			glInternalFormat = GL_COMPRESSED_RGBA;
		} else if (bitmapDataFormat == GL_BGR_EXT) {
			glInternalFormat = GL_COMPRESSED_RGB;
		} else {
			PFC_ASSERT(false);
		}
#else
		GLint glInternalFormat;
		if (cfgTextureCompression)
			glInternalFormat = GL_COMPRESSED_RGB;
		else
			glInternalFormat = GL_RGB;
#endif

		IF_DEBUG(Console::println(L"                                     UPLOAD"));
		
		glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat, width, height, 0, bitmapDataFormat, GL_UNSIGNED_BYTE, data);
		bitmap->UnlockBits(bitmapData.get());
		bitmapData.reset();
		bitmap.reset();
		status = STATUS_UPLOADED;
	}
	LeaveCriticalSection(&uploadCS);
}

void ImgTexture::glDelete(void)
{
	if (glTexture){
		IF_DEBUG(Console::println(L"                                              DELETE"));
		glDeleteTextures(1,glTexture);
		delete[] glTexture;
	} else {
		IF_DEBUG(Console::println(L"                                        EMPTY DELETE"));
	}
	glTexture = 0;
}

float ImgTexture::getAspect()
{
	return aspect;
}

void ImgTexture::setMaxGlTextureSize(int size)
{
	maxGlTextureSize = size;
}


int ImgTexture::getMaxSize(){
	return std::min((int)cfgMaxTextureSize, maxGlTextureSize);
}

void ImgTexture::loadImageFromArt(const album_art_data::ptr &art)
{
	bitmapMemory.Load(art->get_ptr(), art->get_size());
	bitmap = bitmapMemory.stealBitmap();
	prepareUpload();
}

void ImgTexture::loadImageFile(const char * imageFile)
{
	IF_DEBUG(profiler(ImgTexture__loadImageFile));
	bitmap = make_unique<Bitmap>(pfc::stringcvt::string_wide_from_utf8(imageFile));
	if ((bitmap->GetLastStatus() != Ok) ||
		(1 > (bitmap->GetWidth())) ||
		(1 > (bitmap->GetHeight()))){
		bitmap = getErrorBitmap();
	}
	prepareUpload();
}

void ImgTexture::loadImageResource(WORD resource, LPCTSTR type)
{
	bitmapResource.Load(resource, type, core_api::get_my_instance());
	bitmap = bitmapResource.stealBitmap();
	prepareUpload();
}

void ImgTexture::prepareUpload(void)
{
	IF_DEBUG(profiler(ImgTexture__prepareUpload));
	int maxSize = getMaxSize();
	int width = bitmap->GetWidth();;
	int height = bitmap->GetHeight();;

	aspect = float(width)/height;

	// scale textures to maxSize
	if ((width > maxSize) || (height > maxSize)){
		if (width > height){
			height = int(maxSize / aspect);
			width  = maxSize;
		} else {
			width  = int(aspect * maxSize);
			height = maxSize;
		}
	}
	
	// turn texture sizes into powers of two
	int p2w = 1;
	while (p2w < width)
		p2w = p2w << 1;
	width = p2w;
	int p2h = 1;
	while (p2h < height)
		p2h = p2h << 1;
	height = p2h;

	{
		IF_DEBUG(profiler(ImgTexture__prepareUpload__resize));
		unique_ptr<Bitmap> oldBitmap = std::move(bitmap);
		bitmap = make_unique<Bitmap>(width,height);
		Graphics resizer(bitmap.get());
		auto ip = resizer.GetInterpolationMode();
		resizer.SetTransform(&Gdiplus::Matrix( // Flip Y coordinates for OpenGL
			1, 0, 
			0, -1,
			0, float(height)));
		resizer.DrawImage(oldBitmap.get(),0,0,width,height);
		resizer.Flush();
	}

#ifdef COVER_ALPHA
	PixelForamt texFmt;
	PixelFormat imgFmt = bitmap->GetPixelFormat();
	if (IsAlphaPixelFormat(imgFmt) || IsIndexedPixelFormat(imgFmt)) {
		bitmapDataFormat = GL_BGRA_EXT;
		texFmt = PixelFormat32bppARGB;
	} else {
		bitmapDataFormat = GL_BGR_EXT;
		texFmt = PixelFormat24bppRGB;
	}
#else
	static const PixelFormat texFmt = PixelFormat24bppRGB;
	bitmapDataFormat = GL_BGR_EXT;
#endif

	{
		IF_DEBUG(profiler(ImgTexture__prepareUpload__lockBits));
		Rect rc(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
		bitmapData = make_unique<BitmapData>();
		bitmap->LockBits(&rc, ImageLockModeRead, texFmt, bitmapData.get());
	}
	status = STATUS_IMG_LOCKED;
}
