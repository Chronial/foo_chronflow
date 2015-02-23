#pragma once
#include "AlbumCollection.h"
#include "Helpers.h"

#include <unordered_map>

class AppInstance;

class DbAlbum : public service_base
{
	FB2K_MAKE_SERVICE_INTERFACE(DbAlbum, service_base);
public:
	DbAlbum(std::wstring* sortString, pfc::string8 &&findAsYouType) : // steals the strings!
		sortString(std::move(*sortString)), findAsYouType(std::move(findAsYouType)){};
	metadb_handle_list tracks;
	const std::wstring sortString;
	const pfc::string8 findAsYouType;
	int index;
};
// {4F88C012-1F1A-4A1D-A32F-D36C57B4A437}
FOOGUIDDECL const GUID DbAlbum::class_guid = { 0x4f88c012, 0x1f1a, 0x4a1d, { 0xa3, 0x2f, 0xd3, 0x6c, 0x57, 0xb4, 0xa4, 0x37 } };

typedef service_ptr_t<DbAlbum> DbAlbumPtr;

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
	std::unordered_map<std::string, DbAlbumPtr> albumMap;
	pfc::list_t<DbAlbumPtr> sortedAlbums;
	pfc::list_t<DbAlbumPtr> findAsYouType;
	service_ptr_t<titleformat_object> albumMapper;

};
