#pragma once
#include "stdafx.h"

#include "CriticalSection.h"
#include "CGdiPlusBitmap.h"

class TextDisplay;

class ImgTexture
{
public:
	friend TextDisplay; // needs to access forcePowerOfTwo - this is a hack!

private:
	ImgTexture();
public:
	ImgTexture(const char* imageFile);
	ImgTexture(WORD resource, LPCTSTR type);
	~ImgTexture(void);

	void glBind(void);
	void glUpload(void);
	void glDelete(void);
	float getAspect();

	static void setMaxGlTextureSize(int size);

private:
	unique_ptr<Gdiplus::Bitmap> getErrorBitmap();
	int getMaxSize();
	float aspect;

	static int maxGlTextureSize;
	unique_ptr<Gdiplus::Bitmap> bitmap;
	CGdiPlusBitmapResource bitmapResource;
	GLuint* glTexture = 0;
	unique_ptr<Gdiplus::BitmapData> bitmapData;
	GLenum bitmapDataFormat; 
	enum  {
		STATUS_NONE = 0,
		STATUS_IMG_LOCKED = 1,
		STATUS_UPLOADED = 2
	} status = STATUS_NONE;
	CRITICAL_SECTION uploadCS;
	void prepareUpload(void);

	void loadImageFile(const char * imageFile);
	void loadImageResource(WORD resource, LPCTSTR type);
	pfc::string8 name;
};