#pragma once
#include "stdafx.h"

struct free_delete {
	void operator()(void* x) { free(x); }
};

class Image {
public:
	typedef unique_ptr<void, free_delete> malloc_ptr;
	int width;
	int height;
	malloc_ptr data;

	Image(malloc_ptr data, int width, int height);

	static Image fromFile(const char* filename);
	static Image fromFileBuffer(const void* buffer, size_t len);
	static Image fromResource(LPCTSTR pName, LPCTSTR pType, HMODULE hInst);
	static Image fromResource(UINT id, LPCTSTR pType, HMODULE hInst);
	static Image fromGdiBitmap(Gdiplus::Bitmap& bitmap);

	Image resize(int width, int height) const;
};

class GLTexture {
public:
	GLTexture(GLuint glTexture, float originalAspect);
	GLTexture(const GLTexture&) = delete;
	GLTexture& operator=(const GLTexture&) = delete;
	GLTexture(GLTexture&&) noexcept;
	GLTexture& operator=(GLTexture&&) noexcept;
	~GLTexture() noexcept;

	void bind() const;
	float getAspect() const;

private:
	GLuint glTexture;
	float originalAspect;
	void glDelete();
};

class UploadReadyImage {
public:
	UploadReadyImage(Image&& src);
	UploadReadyImage(const UploadReadyImage&) = delete;
	UploadReadyImage& operator=(const UploadReadyImage&) = delete;
	UploadReadyImage(UploadReadyImage&& other)
		: originalAspect(other.originalAspect), image(std::move(other.image)) {};
	UploadReadyImage& operator=(UploadReadyImage&&);
	~UploadReadyImage() = default;

	GLTexture upload() const;
private:
	double originalAspect;
	Image image;
};

std::optional<UploadReadyImage> loadAlbumArt(const metadb_handle_list& tracks);
UploadReadyImage loadSpecialArt(WORD resource, pfc::string8 userImage);