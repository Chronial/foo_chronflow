#pragma once
#include "base.h"

class Image {
 public:
  typedef unique_ptr_del<void, &free> malloc_ptr;
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
  void glDelete() noexcept;
};

class UploadReadyImage {
 public:
  UploadReadyImage(Image&& src);
  UploadReadyImage(const UploadReadyImage&) = delete;
  UploadReadyImage& operator=(const UploadReadyImage&) = delete;
  UploadReadyImage(UploadReadyImage&& other)
      : image(std::move(other.image)), originalAspect(other.originalAspect){};
  UploadReadyImage& operator=(UploadReadyImage&&);
  ~UploadReadyImage() = default;

  GLTexture upload() const;

 private:
  Image image;
  double originalAspect;
};

std::optional<UploadReadyImage> loadAlbumArt(const metadb_handle_list& tracks);
UploadReadyImage loadSpecialArt(WORD resource, pfc::string8 userImage);
