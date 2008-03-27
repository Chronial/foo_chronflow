#pragma once

#define WM_COLLECTION_REFRESHED WM_APP+1

class DisplayPosition;
struct CollectionPos;
class ImgTexture;

class __declspec(novtable) AlbumCollection abstract
{
public:
	/*AlbumCollection(void) = 0;*/
	virtual ~AlbumCollection(void){};
	virtual int getCount(void) = 0;
	virtual ImgTexture* getImgTexture(CollectionPos pos) = 0;
	virtual void getTitle(CollectionPos pos, pfc::string_base& out) = 0;
	virtual metadb_handle_list getTracks(CollectionPos pos) = 0;
	virtual void reloadAsynchStart(bool hardRefresh = false) = 0;
	virtual void reloadAsynchFinish(LPARAM worker) = 0;
    virtual bool getAlbumForTrack(const metadb_handle_ptr& track, CollectionPos& out) = 0;
};