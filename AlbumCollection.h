#pragma once

class __declspec(novtable) AlbumCollection abstract
{
public:
	/*AlbumCollection(void) = 0;*/
	virtual ~AlbumCollection(void){};
	virtual int getCount(void) = 0;
	virtual ImgTexture* getImgTexture(CollectionPos pos) = 0;
	virtual char* getTitle(CollectionPos pos) = 0;
};