#pragma once
#include "AlbumCollection.h"
#include "Helpers.h"

class AppInstance;

class DbAlbumCollection :
	public AlbumCollection
{
	CriticalSection renderThreadCS;

	class RefreshWorker;
	friend class RefreshWorker;
	bool isRefreshing; // unsures that only one RefreshWorker is running at a time
	AppInstance* appInstance;
public:
	DbAlbumCollection(AppInstance* instance);
	~DbAlbumCollection(void);
	inline int getCount() { return albums.get_count();	};
	ImgTexture* getImgTexture(CollectionPos pos);
	void getTitle(CollectionPos pos, pfc::string_base& out);

	int getTracks(CollectionPos pos, metadb_handle_list& out);

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


	/******************************* INTERN DATABASE ***************************/
	/*typedef struct {
		metadb_handle_ptr aTrack;
	} t_album;*/
	pfc::list_t<metadb_handle_ptr> albums;
	struct t_ptrAlbumGroup {
		metadb_handle_ptr ptr;
		pfc::string8 * group;
		int groupId;
	};
	pfc::list_t<t_ptrAlbumGroup> ptrGroupMap;

	class ptrGroupMap_compareGroupId : public pfc::list_base_t<t_ptrAlbumGroup>::sort_callback {
	public:
		int compare(const t_ptrAlbumGroup &a, const t_ptrAlbumGroup &b){
			return b.groupId - a.groupId;
		}
	};
	class ptrGroupMap_comparePtr : public pfc::list_base_t<t_ptrAlbumGroup>::sort_callback {
	public:
		int compare(const t_ptrAlbumGroup &a, const t_ptrAlbumGroup &b){
			return b.ptr.get_ptr() - a.ptr.get_ptr();
		}
	};
	static int ptrGroupMap_searchPtr(const t_ptrAlbumGroup& a, const metadb_handle_ptr& ptr){
		return ptr.get_ptr() - a.ptr.get_ptr();
	}
	class ptrGroupMap_compareGroup : public pfc::list_base_t<t_ptrAlbumGroup>::sort_callback {
	public:
		int compare(const t_ptrAlbumGroup &a, const t_ptrAlbumGroup &b){
			return stricmp_utf8(a.group->get_ptr(), b.group->get_ptr());
		}
	};
	
	struct t_titleAlbumGroup {
		pfc::string8 title;
		int groupId;
	};
	pfc::list_t<t_titleAlbumGroup> titleGroupMap;
	class titleGroupMap_compareTitle : public pfc::list_base_t<t_titleAlbumGroup>::sort_callback {
	public:
		int compare(const t_titleAlbumGroup &a, const t_titleAlbumGroup &b){
			return stricmp_utf8(a.title.get_ptr(), b.title.get_ptr());
		}
	};
};