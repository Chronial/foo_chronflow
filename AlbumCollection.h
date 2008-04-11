#pragma once

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
	virtual int getTracks(CollectionPos pos, metadb_handle_list& out) = 0;
	virtual void reloadAsynchStart(bool hardRefresh = false) = 0;
	virtual void reloadAsynchFinish(LPARAM worker) = 0;
    virtual bool getAlbumForTrack(const metadb_handle_ptr& track, CollectionPos& out) = 0;
};