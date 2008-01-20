#include "chronflow.h"
#include <stdio.h>

DirAlbumCollection::DirAlbumCollection(bool noInit)
{
	if (noInit)
		return;
	albumCount = 0;

	wchar_t imgFile[512]    = {0};
	WIN32_FIND_DATAW FindFileData;
	HANDLE hFind;
	
	hFind = FindFirstFileW(L"M:/Alben/*",&FindFileData);
	int i=0;
	while (true){
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
			swprintf_s(imgFile,512, L"M:/Alben/%s/folder.jpg", FindFileData.cFileName);
			if (GetFileAttributesW(imgFile) != INVALID_FILE_ATTRIBUTES){
				wchar_t* tmpstr = new wchar_t[wcslen(imgFile)+1];
				memcpy(tmpstr,imgFile,(wcslen(imgFile)+1)*sizeof(wchar_t));
				imageArray[i] = tmpstr;

				char * title = new char[wcslen(FindFileData.cFileName)+1];
				WideCharToMultiByte(CP_ACP,NULL,FindFileData.cFileName,-1,title,wcslen(FindFileData.cFileName)+1,NULL,NULL);
				titleArray[i] = title;

				wchar_t * path = new wchar_t[512];
				swprintf_s(path,512, L"\"M:/Alben/%s\"", FindFileData.cFileName);
				pathArray[i] = path;

				i++;
				albumCount++;
				if (i == IMAGE_ARRAY_SIZE)
					break;
			}
		}
		if (!FindNextFileW(hFind,&FindFileData))
			break;
	}
	FindClose(hFind);
}

DirAlbumCollection::~DirAlbumCollection(void)
{
}

int DirAlbumCollection::getCount(void)
{
	return albumCount;
}


char* DirAlbumCollection::getTitle(CollectionPos pos)
{
	return titleArray[pos.toIndex()];
}

ImgTexture* DirAlbumCollection::getImgTexture(CollectionPos pos)
{
	/*static ImgTexture* cache[4000];
	static bool initialized = false;
	if (!initialized)
		ZeroMemory(cache,sizeof(cache)), initialized=true;
	if (cache[pos.toIndex()] == 0)
		cache[pos.toIndex()] = new ImgTexture(imageArray[pos.toIndex()]);
	return cache[pos.toIndex()];*/
	
	return new ImgFileTexture(imageArray[pos.toIndex()]);
}
