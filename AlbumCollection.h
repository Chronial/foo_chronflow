#pragma once

#define WM_COLLECTION_REFRESHED WM_APP+1

class DisplayPosition;

class __declspec(novtable) AlbumCollection abstract
{
public:
	/*AlbumCollection(void) = 0;*/
	virtual ~AlbumCollection(void){};
	virtual int getCount(void) = 0;
	virtual ImgTexture* getImgTexture(CollectionPos pos) = 0;
	virtual char* getTitle(CollectionPos pos) = 0;
	virtual metadb_handle_list getTracks(CollectionPos pos) = 0;
	virtual void reloadAsynchStart(HWND notifyWnd, bool hardRefresh = false) = 0;
	virtual void reloadAsynchFinish(LPARAM worker, DisplayPosition* dPos) = 0;
    virtual bool getAlbumForTrack(const metadb_handle_ptr& track, CollectionPos& out) = 0;
};