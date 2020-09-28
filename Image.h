#pragma once
#include "utils.h"

class Image {
 public:
  using malloc_ptr = unique_ptr_del<void, &free>;
  int width;
  int height;
  bool alpha;
  malloc_ptr data;

  Image(malloc_ptr data, int width, int height, bool alpha);

  static Image fromFile(const char* filename);
  static Image fromFileBuffer(const void* buffer, size_t len);
  static Image fromResource(LPCTSTR pName, LPCTSTR pType, HMODULE hInst);
  static Image fromResource(UINT id, LPCTSTR pType, HMODULE hInst);
  static Image fromGdiBitmap(Gdiplus::Bitmap& bitmap);

  Image resize(int width, int height) const;
};

class GLTexture {
 public:
  GLTexture();
  GLTexture(const GLTexture&) = delete;
  GLTexture& operator=(const GLTexture&) = delete;
  GLTexture(GLTexture&&) noexcept;
  GLTexture(GLTexture&&, bool hasalpha) noexcept;
  GLTexture& operator=(GLTexture&&) noexcept;
  ~GLTexture() noexcept;

  void bind() const;
  bool getAlpha() const { return hasAlpha; }
  void setAlpha(bool hasAlpha) { this->hasAlpha = hasAlpha; }
 private:
  void reset() noexcept;
  GLuint glTexture = 0;
  bool hasAlpha = false;
};

class GLImage {
 public:
  GLImage(GLTexture glTexture, float originalAspect, bool hasAlpha)
      : glTexture(std::move(glTexture), hasAlpha), originalAspect(originalAspect), hasAlpha(hasAlpha){
    glTexture.setAlpha(hasAlpha);
  };

  void bind() const { glTexture.bind(); };
  float getAspect() const { return originalAspect; };
  bool getAlpha() const {
    //return hasAlpha;
    return glTexture.getAlpha();
  }

 private:
  GLTexture glTexture;
  float originalAspect;
  bool hasAlpha;
};

class UploadReadyImage {
 public:
  explicit UploadReadyImage(Image&& src);
  UploadReadyImage(const UploadReadyImage&) = delete;
  UploadReadyImage& operator=(const UploadReadyImage&) = delete;
  UploadReadyImage(UploadReadyImage&& other)
      : image(std::move(other.image)), originalAspect(other.originalAspect){};
  UploadReadyImage& operator=(UploadReadyImage&&);
  ~UploadReadyImage() = default;

  GLImage upload() const;

 private:
  Image image;
  double originalAspect;
};

std::optional<UploadReadyImage> loadAlbumArt(const metadb_handle_ptr& track,
                                             abort_callback& abort);
std::optional<UploadReadyImage> loadAlbumArtv2(const metadb_handle_ptr & track, const unsigned int coverart,
                                             abort_callback & abort);
UploadReadyImage loadSpecialArt(WORD resource, pfc::string8 userImage, bool hasAlpha);

GLImage loadSpinner();
