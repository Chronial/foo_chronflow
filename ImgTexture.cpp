#include "stdafx.h"
#include "base.h"
#include "config.h"

#include "ImgTexture.h"

#include "Console.h"

#include "Helpers.h"

int  ImgTexture::maxGlTextureSize = 512;

ImgTexture::ImgTexture(const char * imageFile){
	try {
		image = Image::fromFile(imageFile);
	} catch (...){
		image = getErrorImage(imageFile);
	}
	prepareUpload();
}

ImgTexture::ImgTexture(WORD resource, LPCTSTR type){
	try {
		image = Image::fromResource(resource, type, core_api::get_my_instance());
	} catch (...){
		image = getErrorImage("interal image");
	}
	prepareUpload();
}

ImgTexture::ImgTexture(const album_art_data::ptr &art){
	try {
		image = Image::fromFileBuffer(art->get_ptr(), art->get_size());
	} catch (...){
		image = getErrorImage("album art");
	}
	prepareUpload();
}

ImgTexture::~ImgTexture(void)
{
	if (glTexture){
		IF_DEBUG(__debugbreak());
	}
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

Image ImgTexture::getErrorImage(const char* name){
	using namespace Gdiplus;
	const int size = getMaxSize();
	Bitmap bitmap{size, size};
	Graphics drawer{&bitmap};
	SolidBrush blackBrush{Color{0, 0, 0}};
	drawer.FillRectangle(&blackBrush, Rect{0, 0, size, size});
	Pen redPen{Color{255, 0, 0}, size / 10.0f};
	int border = int(size * 0.2f);
	drawer.DrawLine(&redPen, border, border, size - border, size - border);
	drawer.DrawLine(&redPen, border, size - border, size - border, border);

	SolidBrush whiteBrush{Color{255, 255, 255}};
	Gdiplus::Font font{L"Verdana", static_cast<REAL>(size / 256.0 * 15.0)};
	StringFormat format;
	format.SetAlignment(StringAlignmentCenter);
	format.SetLineAlignment(StringAlignmentCenter);
	pfc::string_formatter message;
	message << "Failed to load \n" << name;
	drawer.DrawString(
		pfc::stringcvt::string_wide_from_utf8{message}, -1,
		&font, RectF{0, 0, float(size), float(size)}, &format, &whiteBrush);

	return ::Image::fromGdiBitmap(bitmap);
}

void ImgTexture::glUpload(void)
{
	TRACK_CALL_TEXT("ImgTexture::glUpload");
	IF_DEBUG(profiler(ImgTexture__glUpload));
	IF_DEBUG(double preLoad = Helpers::getHighresTimer());
	if ((status == STATUS_IMG_LOCKED) && image){
		glTexture = new GLuint[1];
		glGenTextures(1, glTexture);
		glBindTexture(GL_TEXTURE_2D, glTexture[0]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);

		GLint glInternalFormat;
		if (cfgTextureCompression)
			glInternalFormat = GL_COMPRESSED_RGB;
		else
			glInternalFormat = GL_RGB;

		glTexImage2D(
			GL_TEXTURE_2D, 0, glInternalFormat,
			image->width, image->height, 0, GL_RGB, GL_UNSIGNED_BYTE,
			image->data);
		status = STATUS_UPLOADED;
		image = boost::none;
		IF_DEBUG(Console::printf(L"                                     UPLOAD (%d us)\n", int((Helpers::getHighresTimer() - preLoad) * 1000 * 1000)));
	}
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

void ImgTexture::prepareUpload(void)
{
	IF_DEBUG(profiler(ImgTexture__prepareUpload));
	const int maxSize = getMaxSize();

	int width = image->width;
	int height = image->height;
	aspect = float(width) / height;

	// scale textures to maxSize
	if ((width > maxSize) || (height > maxSize)){
		if (width > height){
			height = int(maxSize / aspect);
			width = maxSize;
		} else {
			width = int(aspect * maxSize);
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

	if (width != image->width || height != image->height){
		image = image->resize(width, height);
	}
	status = STATUS_IMG_LOCKED;
}
