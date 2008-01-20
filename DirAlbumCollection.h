#pragma once
#include "albumcollection.h"

class DirAlbumCollection :
	public AlbumCollection
{
public:
	DirAlbumCollection(bool noInit = false);
public:
	virtual ~DirAlbumCollection(void);
	virtual int getCount(void);
	virtual char* getTitle(CollectionPos pos);
	virtual ImgTexture* getImgTexture(CollectionPos pos);

protected:
	int albumCount;
	static const int IMAGE_ARRAY_SIZE = 5000;
	wchar_t* imageArray[IMAGE_ARRAY_SIZE];
	wchar_t* pathArray[IMAGE_ARRAY_SIZE];
	char* titleArray[IMAGE_ARRAY_SIZE];
};