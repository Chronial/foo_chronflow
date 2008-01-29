#pragma once

class ImgTexture
{
public:
	ImgTexture(const char* imageFile);
public:
	~ImgTexture(void);
public:
	void glBind(void);
public:
	void glUpload(void);
public:
	void glDelete(void);
public:
	float getAspect();

public:
	static void setForcePowerOfTwo(bool force = true);
public:
	static void setMaxGlTextureSize(int size);

public:
	const char* getIdentifier();

private:
	Gdiplus::Bitmap* getErrorBitmap();
	int getMaxSize();
	float aspect;

	static bool forcePowerOfTwo;
	static int maxGlTextureSize;
	Gdiplus::Bitmap* bitmap;
	GLuint* glTexture;
	Gdiplus::BitmapData* bitmapData;

	enum  {
		STATUS_NONE = 0,
		STATUS_IMG_LOCKED = 1,
		STATUS_UPLOADED = 2
	} status;
	void prepareUpload(void);

	void loadImage(void);
	pfc::string8 imageFile;
};