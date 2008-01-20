#pragma once
#include "imgtexture.h"

class ImgFileTexture :
	public ImgTexture
{
public:
	ImgFileTexture(const wchar_t* imageFile);
public:
	virtual ~ImgFileTexture(void);
protected:
	virtual int getMaxSize();
private:
	void loadImage(void);
	wchar_t imageFile[MAX_PATH];

};
