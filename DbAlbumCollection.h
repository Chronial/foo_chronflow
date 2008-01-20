#pragma once

class DbAlbumCollection :
	public AlbumCollection
{
public:
	DbAlbumCollection(void);
	~DbAlbumCollection(void);
	int getCount(void);
	ImgTexture* getImgTexture(CollectionPos pos);
	char* getTitle(CollectionPos pos);

private:
	pfc::list_t<pfc::string8> imageList;
};
