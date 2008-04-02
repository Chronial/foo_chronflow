#pragma once


class DbAlbumCollection :
	public AlbumCollection
{
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
	void reloadAsynchStart(bool hardRefresh = false);
	void reloadAsynchFinish(LPARAM worker);
	void reloadSourceScripts();

private:
	bool getImageForTrack(const metadb_handle_ptr &track, pfc::string_base &out);

	pfc::list_t< service_ptr_t<titleformat_object> > sourceScripts;
	CRITICAL_SECTION sourceScriptsCS;

	/*typedef struct {
		metadb_handle_ptr aTrack;
	} t_album;*/
	pfc::list_t<metadb_handle_ptr> albums;
	struct t_ptrAlbumGroup {
		metadb_handle_ptr ptr;
		pfc::string8_fastalloc * group;
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
		/*static int bTreeCompare(const t_ptrAlbumGroup &a, const t_ptrAlbumGroup &b){
			return b.ptr.get_ptr() - a.ptr.get_ptr();
		}*/
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
};