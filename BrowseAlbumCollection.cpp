#include "chronflow.h"
#include "shlobj.h"
#include <stdio.h>

BrowseAlbumCollection::BrowseAlbumCollection(void) : DirAlbumCollection(true)
{
	bool repeat;
	do {
		BROWSEINFO bi = {0};
		LPITEMIDLIST pidl;
		wchar_t szDisplay[MAX_PATH];
		wchar_t rootPath[MAX_PATH];

		CoInitialize(NULL);

		bi.pszDisplayName = szDisplay;
		bi.lpszTitle = TEXT("Please choose your collection's root folder.");
		bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON;

		pidl = SHBrowseForFolder(&bi);

		if (NULL != pidl){
			BOOL noError;
			noError = SHGetPathFromIDList(pidl, rootPath);
			CoTaskMemFree(pidl);
			if (!noError)
				exit(0);
		} else {
			exit(0);
		}
		CoUninitialize();

		albumCount = 0;
		scanDirectory(rootPath);
		if (albumCount == 0){
			int res = MessageBox(NULL,L"No Images where found in the specified directory, please choose another one.",L"Error",MB_OKCANCEL|MB_ICONINFORMATION);
			if (res == IDOK)
				repeat = true;
			else
				exit(0);
		} else {
			repeat = false;
		}
	} while (repeat);
}

void BrowseAlbumCollection::scanDirectory(wchar_t *path){
	wchar_t tPath[MAX_PATH];
	WIN32_FIND_DATAW FindFileData;
	HANDLE hFind;
	
	
	swprintf_s(tPath,MAX_PATH, L"%s\\*", path);
	hFind = FindFirstFileW(tPath,&FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
		return;
	FindNextFileW(hFind,&FindFileData);
	while (FindNextFileW(hFind,&FindFileData)){
		wchar_t imgFile[MAX_PATH];
		swprintf_s(imgFile,MAX_PATH, L"%s\\%s", path, FindFileData.cFileName);

		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
			scanDirectory(imgFile);
		} else {
			if (albumCount == IMAGE_ARRAY_SIZE)
				break;
			if (GetFileAttributesW(imgFile) != INVALID_FILE_ATTRIBUTES){
				static LPWSTR* imgTypes = 0;// MEMORY LEAK - THIS HAS TO BE FREED ON DESTRUCTION
				static int imgTypesCount = 0;
				static bool cmlRead = false;
				if (!cmlRead){
					LPWSTR* args = CommandLineToArgvW(GetCommandLineW(),&imgTypesCount);
					imgTypesCount--;
					imgTypes = args + 1;
					cmlRead = true;
				}
				
				bool rightImgType = true;
				if (imgTypesCount > 0){
					rightImgType = false;
					for (int i=0; i < imgTypesCount; i++){
						if (_wcsicmp(FindFileData.cFileName,imgTypes[i]) == 0)
							rightImgType = true;
					}
				}

				wchar_t* ext = wcsrchr(imgFile,L'.');
				
				if (rightImgType &&
					(ext != 0) &&
					(_wcsicmp(ext, L".jpg") == 0 ||
					_wcsicmp(ext, L".jpeg") == 0 ||
					_wcsicmp(ext, L".png") == 0 ||
					_wcsicmp(ext, L".gif") == 0 ||
					_wcsicmp(ext, L".bmp") == 0)){
					wchar_t* tmpstr = new wchar_t[wcslen(imgFile)+1]; // Memory Leak - is never freed
					memcpy(tmpstr,imgFile,(wcslen(imgFile)+1)*sizeof(wchar_t));
					imageArray[albumCount] = tmpstr;

					/*char * title = new char[wcslen(FindFileData.cFileName)+1];
					WideCharToMultiByte(CP_ACP,NULL,FindFileData.cFileName,-1,title,wcslen(FindFileData.cFileName)+1,NULL,NULL);
					titleArray[albumCount] = title;*/

					wchar_t * aPath = new wchar_t[512];
					swprintf_s(aPath,512, L"\"%s\"", path);
					pathArray[albumCount] = aPath;

					albumCount++;
				}
			}
		}
	}
	FindClose(hFind);
}

BrowseAlbumCollection::~BrowseAlbumCollection(void)
{
}
