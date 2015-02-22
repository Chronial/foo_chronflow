#pragma once
#include "AlbumCollection.h"
#include "Helpers.h"

#include <unordered_map>

class AppInstance;

struct DbAlbum {
	metadb_handle_list tracks;
	std::wstring sortString;
	pfc::string8 findAsYouType;
	int index;
};

class DbAlbumCollection :
	public AlbumCollection
{
	CriticalSection renderThreadCS;

	class RefreshWorker;
	friend class RefreshWorker;
	bool isRefreshing; // ensures that only one RefreshWorker is running at a time
	AppInstance* appInstance;
public:
	DbAlbumCollection(AppInstance* instance);
	~DbAlbumCollection(void);
	inline int getCount() { return albumMap.size(); };
	ImgTexture* getImgTexture(CollectionPos pos);
	void getTitle(CollectionPos pos, pfc::string_base& out);

	bool getTracks(CollectionPos pos, metadb_handle_list& out);

	bool getAlbumForTrack(const metadb_handle_ptr& track, CollectionPos& out);

	// This will modify o_min and o_max only if it returns true
	// o_min is the first element that might match, o_max is the frist element that will not match
	// to search the whole collection, set o_min=0 and o_max=~0
	bool searchAlbumByTitle(const char * title, t_size& o_min, t_size& o_max, CollectionPos& out);

	void reloadAsynchStart(bool hardRefresh = false);
	void reloadAsynchFinish(LPARAM worker);

private:
	void reloadSourceScripts();

private:
	bool getImageForTrack(const metadb_handle_ptr &track, pfc::string_base &out);

	pfc::list_t< service_ptr_t<titleformat_object> > sourceScripts;
	CRITICAL_SECTION sourceScriptsCS;

	

private:
	/******************************* INTERN DATABASE ***************************/
	std::unordered_map<std::string, DbAlbum> albumMap;
	pfc::list_t<DbAlbum*> sortedAlbums;
	pfc::list_t<DbAlbum*> findAsYouType;
	service_ptr_t<titleformat_object> albumMapper;
};
