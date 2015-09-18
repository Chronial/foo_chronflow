#pragma once
#include "AlbumCollection.h"
#include "Helpers.h"

#include <Shlwapi.h>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ranked_index.hpp>
#include <boost/multi_index/member.hpp>

#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
using namespace boost::multi_index;

class AppInstance;

struct DbAlbum
{
	std::string groupString;
	std::wstring sortString;
	pfc::string8 findAsYouType;
	metadb_handle_list tracks;
};

struct CompWLogical
{
	bool operator()(std::wstring a, std::wstring b)const{
		return StrCmpLogicalW(a.data(), b.data()) < 0;
	}
};

struct CompIUtf8
{
	bool operator()(pfc::string8 a, pfc::string8 b)const{
		return stricmp_utf8(a, b) < 0;
	}
};


typedef multi_index_container<
	DbAlbum,
	indexed_by<
		hashed_unique< member<DbAlbum, std::string, &DbAlbum::groupString> >,
		ranked_non_unique<
			member<DbAlbum, std::wstring, &DbAlbum::sortString>,
			CompWLogical
		>,
		ordered_non_unique<
			member<DbAlbum, pfc::string8, &DbAlbum::findAsYouType>,
			CompIUtf8
		>
	>
> DbAlbums;


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
	inline int getCount() { return albums.size(); };
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
	DbAlbums albums;
	service_ptr_t<titleformat_object> albumMapper;
};
