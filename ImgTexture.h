#pragma once
#include "stdafx.h"

#include "CriticalSection.h"

#include "Image.h"

class TextDisplay;

class ImgTexture
{
public:
	friend TextDisplay; // needs to access forcePowerOfTwo - this is a hack!

	ImgTexture(const char* imageFile);
	ImgTexture(WORD resource, LPCTSTR type);
	ImgTexture(const album_art_data::ptr &art);
	~ImgTexture(void);

	void glBind(void);
	void glUpload(void);
	void glDelete(void);
	float getAspect();

	static void setMaxGlTextureSize(int size);

private:
	Image getErrorImage(const char* name);
	int getMaxSize();
	float aspect;
	boost::optional<Image> image;

	static int maxGlTextureSize;
	GLuint* glTexture = 0;
	enum  {
		STATUS_NONE = 0,
		STATUS_IMG_LOCKED = 1,
		STATUS_UPLOADED = 2
	} status = STATUS_NONE;
	void prepareUpload(void);
};