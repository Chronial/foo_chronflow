#pragma once
#include "stdafx.h"

#include "stb_image.h"
#include "stb_image_resize.h"


class Image {
public:
	int width;
	int height;
	gsl::owner<void*> data;

	explicit Image(gsl::owner<void*> data, int width, int height);
	Image(const Image&) = delete;
	Image& operator=(const Image&) = delete;
	Image(Image&&) throw();
	Image& operator=(Image&&) throw();
	~Image() throw();

	static Image fromFile(const char* filename);
	static Image fromFileBuffer(const void* buffer, size_t len);
	static Image fromResource(LPCTSTR pName, LPCTSTR pType, HMODULE hInst);
	static Image fromResource(UINT id, LPCTSTR pType, HMODULE hInst);
	static Image fromGdiBitmap(Gdiplus::Bitmap& bitmap);

	Image resize(int width, int height);
};