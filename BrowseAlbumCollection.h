#pragma once
#include "diralbumcollection.h"

class BrowseAlbumCollection :
	public DirAlbumCollection
{
public:
	BrowseAlbumCollection(void);
public:
	~BrowseAlbumCollection(void);
private:
	void scanDirectory(TCHAR* path);
};
