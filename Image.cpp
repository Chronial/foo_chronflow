#include "stdafx.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "Image.h"


Image::Image(gsl::owner<void*> data, int width, int height)
	: data(data), width(width), height(height) {}

Image::Image(Image&& other)
	: data(other.data), width(other.width), height(other.height)
{
	other.data = nullptr;
}

Image& Image::operator=(Image&& other){
	free(data);
	width = other.width;
	height = other.height;
	data = other.data;
	other.data = nullptr;
	return *this;
}

Image::~Image(){
	free(data);
}

Image Image::fromFile(const char* filename){
	auto wideName = pfc::stringcvt::string_wide_from_utf8(filename);
	int width;
	int height;
	int channels_in_file;
	FILE* f;
	stbi_set_flip_vertically_on_load(true);
	if (!_wfopen_s(&f, wideName, L"r"))
		throw std::runtime_error{"Failed to open image file"};
	void* data = stbi_load_from_file(
		f,
		&width, &height, &channels_in_file, 3);
	fclose(f);
	if (data == nullptr){
		throw std::runtime_error{"Failed to load image file"};
	}
	return Image{data, width, height};
}

Image Image::fromFileBuffer(const void* buffer, size_t len){
	int width;
	int height;
	int channels_in_file;
	stbi_set_flip_vertically_on_load(true);
	void* data = stbi_load_from_memory(
		static_cast<const stbi_uc*>(buffer), len,
		&width, &height, &channels_in_file, 3);
	if (data == nullptr){
		throw std::runtime_error{"Failed to load image buffer"};
	}
	return Image{data, width, height};
}

Image Image::fromResource(LPCTSTR pName, LPCTSTR pType, HMODULE hInst){
	HRSRC hResource = FindResource(hInst, pName, pType);
	if (!hResource)
		throw std::runtime_error{"Failed to find image resource"};

	DWORD resourceSize = SizeofResource(hInst, hResource);
	if (!resourceSize)
		throw std::runtime_error{"Failed to size image resource"};

	HGLOBAL resourceHandle = LoadResource(hInst, hResource);
	if (!resourceHandle)
		throw std::runtime_error{"Failed to load image resource"};

	const void* resourceData = LockResource(resourceHandle);
	if (!resourceData)
		throw std::runtime_error{"Failed to lock image resource"};

	return Image::fromFileBuffer(resourceData, resourceSize);
}

Image Image::fromResource(UINT id, LPCTSTR pType, HMODULE hInst){
	return Image::fromResource(MAKEINTRESOURCE(id), pType, hInst);
}

Image Image::fromGdiBitmap(Gdiplus::Bitmap& bitmap){
	using namespace Gdiplus;
	BitmapData bitmapData;
	Rect rc(0, 0, bitmap.GetWidth(), bitmap.GetHeight());
	if (Ok != bitmap.LockBits(&rc, ImageLockModeRead, PixelFormat24bppRGB, &bitmapData)){
		throw std::runtime_error{"Failed to lock bitmap data"};
	}
	auto _ = gsl::finally([&] { bitmap.UnlockBits(&bitmapData); });
	size_t bufferSize = bitmapData.Width * bitmapData.Height * 3;
	void* outBuffer = malloc(bufferSize);
	if (outBuffer == nullptr){
		throw std::bad_alloc{};
	}
	memcpy(outBuffer, bitmapData.Scan0, bufferSize);
	stbi__vertical_flip(outBuffer, bitmapData.Width, bitmapData.Height, 3);
	
	// convert bgr to rgb
	uint8_t* p = static_cast<uint8_t*>(outBuffer);
	for (t_size i = 0; i < bitmapData.Width * bitmapData.Height; ++i) {
		stbi_uc t = p[0];
		p[0] = p[2];
		p[2] = t;
		p += 3;
	}

	return Image(outBuffer, bitmapData.Width, bitmapData.Height);
}

Image Image::resize(int width, int height){
	size_t new_size = width * height * 3;
	stbi_uc* new_buffer = static_cast<stbi_uc*>(malloc(new_size));
	if (new_buffer == nullptr){
		throw std::bad_alloc{};
	}
	int result = stbir_resize_uint8_srgb(
		static_cast<stbi_uc*>(data), this->width, this->height, 0,
		new_buffer, width, height, 0,
		3, STBIR_ALPHA_CHANNEL_NONE, 0);
	if (result != 1){
		free(new_buffer);
		throw std::runtime_error{ "Failed to resize image" };
	}
	return Image{new_buffer, width, height};
}