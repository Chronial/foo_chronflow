#include "stdafx.h"

#include "DbAlbumCollection.h"

#include "AsynchTexLoader.h"
#include "AppInstance.h"
#include "DisplayPosition.h"
#include "ImgTexture.h"

#include <process.h>

extern cfg_string cfgGroup;
extern cfg_string cfgFilter;
extern cfg_string cfgSources;
extern cfg_string cfgSort;
extern cfg_string cfgInnerSort;
extern cfg_bool cfgSortGroup;
extern cfg_string cfgAlbumTitle;
extern service_ptr_t<titleformat_object> cfgAlbumTitleScript;


namespace {
	struct custom_sort_data	{
		HANDLE text;
		t_size index;
	};
}
static int __cdecl custom_sort_compare(const custom_sort_data & elem1, const custom_sort_data & elem2){
	int ret = uSortStringCompare(elem1.text,elem2.text);
	if (ret == 0) ret = pfc::sgn_t((t_ssize)elem1.index - (t_ssize)elem2.index);
	return ret;
}


class DbAlbumCollection::RefreshWorker {
private:
	metadb_handle_list library;
	pfc::list_t<DbAlbumCollection::t_ptrAlbumGroup> ptrGroupMap;
	pfc::list_t<metadb_handle_ptr> albums;
	pfc::list_t<DbAlbumCollection::t_titleAlbumGroup> titleGroupMap;

private:
	void copyDataToCollection(DbAlbumCollection* collection){
		ScopeCS scopeLock(collection->renderThreadCS);
		collection->albums = albums;
		collection->ptrGroupMap = ptrGroupMap;
		collection->titleGroupMap = titleGroupMap;
	}
private:
	// call from mainthread!
	void init(){
		static_api_ptr_t<library_manager> lm;
		lm->get_all_items(library);
	}


	void generateData(){
		//collection->reloadSourceScripts();
		static_api_ptr_t<titleformat_compiler> compiler;

		static_api_ptr_t<metadb> db;
		
		t_size count = library.get_count();

		double preFilter = Helpers::getHighresTimer();
		/* // TODO: Rectivate
		if (!cfgFilter.is_empty()){ // filter
			search_tools::search_filter filter;
			filter.init(cfgFilter);
			bit_array_bittable filterMask(count);

			db->database_lock();
			for (t_size i=0; i < count; i++){
				const metadb_handle_ptr f = library.get_item_ref(i);
				if (filter.test(f))
					filterMask.set(i, true);
			}
			db->database_unlock();

			library.filter_mask(filterMask);
			count = library.get_count();
		}
		*/
		console::printf("Filter %d msec",int((Helpers::getHighresTimer() - preFilter)*1000));
		



		service_ptr_t<titleformat_object> groupScript;
		compiler->compile(groupScript, cfgGroup);
		double preGroup = Helpers::getHighresTimer();
		ptrGroupMap.prealloc(count);
		{ // GroupMap
			for (t_size i=0; i < count; i++){
				t_ptrAlbumGroup map;
				map.ptr = library.get_item(i);
				map.group = new pfc::string8;
				map.ptr->format_title(0, *map.group, groupScript, 0);
				ptrGroupMap.add_item(map);
			}
		}
		console::printf("Group %d msec",int((Helpers::getHighresTimer() - preGroup)*1000));


		double prePopulate = Helpers::getHighresTimer();
		albums.prealloc(count);
		{ // Populate albums
			ptrGroupMap.sort(ptrGroupMap_compareGroup());
			t_ptrAlbumGroup* ptrGroupMapPtr = ptrGroupMap.get_ptr();
			pfc::string8 * previousGroup = 0;
			int albumCount = -1;
			for (t_size i=0; i < count; i++){
				t_ptrAlbumGroup* e = ptrGroupMapPtr + i;//ptrGroupMap.get_item(i);
				if (previousGroup == 0 || stricmp_utf8(e->group->get_ptr(),previousGroup->get_ptr()) != 0){
					albumCount++;
					albums.add_item(e->ptr);
					delete previousGroup;
					previousGroup = e->group;
				} else {
					//belongs to previous Group
					delete e->group;
				}
				e->groupId = albumCount;
			}
			delete previousGroup;
			albumCount++;
			
			ptrGroupMap.sort(ptrGroupMap_comparePtr());

			if (!cfgSortGroup){ //Sort albums
				service_ptr_t<titleformat_object> sortScript;
				compiler->compile(sortScript, cfgSort);
				pfc::array_t<t_size> order; order.set_size(albumCount);
				pfc::array_t<t_size> revOrder; revOrder.set_size(albumCount);

				{ // fill Order with correct permutation
					pfc::string8_fastalloc sortOut; sortOut.prealloc(512);
					pfc::array_t<custom_sort_data> sortData; sortData.set_size(albumCount);

					for(int i=0; i < albumCount; i++)
					{
						albums.get_item_ref(i)->format_title(0, sortOut, sortScript, 0);
						sortData[i].index = i;
						sortData[i].text = uSortStringCreate(sortOut);
					}
					pfc::sort_t(sortData, custom_sort_compare, albumCount);

					for(int i=0; i < albumCount; i++){
						order[i] = sortData[i].index;
						revOrder[sortData[i].index] = i;
						uSortStringFree(sortData[i].text);
					}
				}
				albums.reorder(order.get_ptr());

				t_ptrAlbumGroup* ptrGroupMapPtr = ptrGroupMap.get_ptr();
				for (t_size i = 0; i < count; i++){
					t_ptrAlbumGroup* map = ptrGroupMapPtr + i;
					map->groupId = revOrder[map->groupId];
				}
			}

			{ // Generate Map for Find As You Type
				titleGroupMap.set_size(albumCount);
				for (int i=0; i < albumCount; i++){
					titleGroupMap[i].groupId = i;
					albums.get_item_ref(i)->format_title(0, titleGroupMap[i].title, cfgAlbumTitleScript, 0);
				}
				titleGroupMap.sort(titleGroupMap_compareTitle());
			}
		}
		console::printf("Populate %d msec",int((Helpers::getHighresTimer() - prePopulate)*1000));
	}

private:
	AppInstance* appInstance;
	RefreshWorker(AppInstance* instance){
		this->appInstance = instance;
	}
private:
	HANDLE workerThread;
	bool hardRefresh;
	void startWorkerThread()
	{
		workerThread = (HANDLE)_beginthreadex(0,0,&(this->runWorkerThread),(void*)this,0,0);
		SetThreadPriority(workerThread, THREAD_PRIORITY_BELOW_NORMAL);
		SetThreadPriorityBoost(workerThread, true);
	}

	static unsigned int WINAPI runWorkerThread(void* lpParameter)
	{
		((DbAlbumCollection::RefreshWorker*)(lpParameter))->workerThreadProc();
		return 0;
	}

	void workerThreadProc(){
		this->generateData();
		PostMessage(appInstance->mainWindow, WM_COLLECTION_REFRESHED, 0, (LPARAM)this);
		// Notify mainWindow to copy data, after copying, refresh AsynchTexloader + start Loading
	}

public:
	static void reloadAsynchStart(AppInstance* instance, bool hardRefresh = false){
		instance->texLoader->pauseLoading();
		RefreshWorker* worker = new RefreshWorker(instance);
		worker->hardRefresh = hardRefresh;
		worker->init();
		worker->startWorkerThread();
	}
	void reloadAsynchFinish(DbAlbumCollection* collection){
		double a[6];
		a[0] = Helpers::getHighresTimer();
		a[1] = Helpers::getHighresTimer();
		if (hardRefresh){
			collection->reloadSourceScripts();
			appInstance->texLoader->clearCache();
		} else {
			appInstance->texLoader->resynchCache(staticResynchCallback, (void*)this);
		}

		a[2] = Helpers::getHighresTimer();

		{ // update DisplayPosition
			ScopeCS scopeLock(appInstance->displayPos->accessCS);
			CollectionPos centeredPos = appInstance->displayPos->getCenteredPos();
			int centeredIdx = centeredPos.toIndex();
			CollectionPos targetPos = appInstance->displayPos->getTarget();
			int targetIdx = targetPos.toIndex();

			int newCenteredIdx = resynchCallback(centeredIdx, collection);
			if (newCenteredIdx >= 0){
				centeredPos += (newCenteredIdx - centeredIdx);
				appInstance->displayPos->hardSetCenteredPos(centeredPos);
				int newTargetIdx = resynchCallback(targetIdx, collection);
				if (newTargetIdx >= 0){
					targetPos += (newTargetIdx - targetIdx);
				} else {
					targetPos += (newCenteredIdx - centeredIdx);
				}
				appInstance->displayPos->hardSetTarget(targetPos);
				appInstance->texLoader->setQueueCenter(targetPos);
			}
		}
		a[3] = Helpers::getHighresTimer();

		copyDataToCollection(collection);
		a[4] = Helpers::getHighresTimer();
		appInstance->texLoader->resumeLoading();
		a[5] = Helpers::getHighresTimer();
		for (int i=0; i < 5; i++){
			console::printf("Times: a%d - a%d: %dmsec", i, i+1, int((a[i+1] - a[i])*1000));
		}
		appInstance->redrawMainWin();
		delete this;
	}
	static int CALLBACK staticResynchCallback(int oldIdx, void* p_this, AlbumCollection* collection){
		return reinterpret_cast<RefreshWorker*>(p_this)->resynchCallback(oldIdx, collection);
	}
	inline int resynchCallback(int oldIdx, AlbumCollection* collection){
		DbAlbumCollection* col = dynamic_cast<DbAlbumCollection*>(collection);
		//t_ptrAlbumGroup oldMap;
		if (oldIdx+1 > (int)col->albums.get_count())
			return -1;
		//oldMap.ptr = col->albums.get_item_ref(oldIdx);
		metadb_handle_ptr ptr = col->albums.get_item_ref(oldIdx);
		t_size ptrIdx;
		//if (ptrGroupMap.bsearch_t(&(ptrGroupMap_comparePtr::bTreeCompare), oldMap, ptrIdx)){
		if (ptrGroupMap.bsearch_t(&(ptrGroupMap_searchPtr), ptr, ptrIdx)){
			return(ptrGroupMap.get_item(ptrIdx).groupId);
		} else {
			return -1;
		}
	}
};

void DbAlbumCollection::reloadSourceScripts(){
	static_api_ptr_t<titleformat_compiler> compiler;
	EnterCriticalSection(&sourceScriptsCS);
	sourceScripts.remove_all();

	const char * srcStart = cfgSources.get_ptr();
	const char * srcP = srcStart;
	const char * srcEnd;
	for (;; srcP++){
		if (*srcP == '\r' || *srcP == '\n' || *srcP == '\0'){
			srcEnd = (srcP-1);
			while (*(srcEnd) == ' ')
				srcEnd--;
			if (srcEnd > srcStart){
				pfc::string8_fastalloc src;
				src.set_string(srcStart, srcEnd - srcStart + 1);
				service_ptr_t<titleformat_object> srcScript;
				compiler->compile(srcScript, src);
				sourceScripts.add_item(srcScript);
			}
			srcStart = srcP+1;
			if (*srcP == '\0')
				break;
		}
	}
	LeaveCriticalSection(&sourceScriptsCS);
}

DbAlbumCollection::DbAlbumCollection(AppInstance* instance){
	this->appInstance = instance;
	InitializeCriticalSectionAndSpinCount(&sourceScriptsCS, 0x80000400);
	isRefreshing = false;

	static_api_ptr_t<titleformat_compiler> compiler;
	compiler->compile(cfgAlbumTitleScript, cfgAlbumTitle);

	//reloadSourceScripts();
	/*double start = Helpers::getHighresTimer();
	RefreshWorker worker;
	worker.init();
	worker.generateData();
	double mid = Helpers::getHighresTimer();
	worker.copyDataToCollection(this);
	double end = Helpers::getHighresTimer();
	console::printf("Generate %d msec, copy %d msec",int((mid-start)*1000),int((end-mid)*1000));*/
}

void DbAlbumCollection::reloadAsynchStart(bool hardRefresh){
	isRefreshing = true;
	RefreshWorker::reloadAsynchStart(appInstance, hardRefresh);
	if (hardRefresh){
		appInstance->texLoader->loadSpecialTextures();
	}
}

void DbAlbumCollection::reloadAsynchFinish(LPARAM worker){
	double synchStart = Helpers::getHighresTimer();
	reinterpret_cast<RefreshWorker*>(worker)->reloadAsynchFinish(this);
	console::printf("Synch: %d msec (in mainthread!)",int((Helpers::getHighresTimer()-synchStart)*1000));
	isRefreshing = false;
}

bool DbAlbumCollection::getImageForTrack(const metadb_handle_ptr &track, pfc::string_base &out){
	bool imgFound = false;
	//abort_callback_impl abortCallback;

	EnterCriticalSection(&sourceScriptsCS);
	int sourceCount = sourceScripts.get_count();
	for (int j=0; j < sourceCount; j++){
		track->format_title(0, out, sourceScripts.get_item_ref(j), 0);
		Helpers::fixPath(out);
		if (uFileExists(out)){
			imgFound = true;
			break;
		}
		/*try {
			if (filesystem::g_exists(out, abortCallback) &&
				!filesystem::g_is_valid_directory(out, abortCallback) &&
				!filesystem::g_is_remote_or_unrecognized(out)){
					imgFound = true;
					break;
			}
		} catch (exception_io_no_handler_for_path){
		}*/
	}
	LeaveCriticalSection(&sourceScriptsCS);
	return imgFound;
}

int DbAlbumCollection::getTracks(CollectionPos pos, metadb_handle_list& out){
	int trackCount = ptrGroupMap.get_count();
	int group = pos.toIndex();
	out.remove_all();
	int c = 0;
	for (int i=0; i < trackCount; i++){
		if (ptrGroupMap.get_item_ref(i).groupId == group){
			out.add_item(ptrGroupMap.get_item_ref(i).ptr);
			c++;
		}
	}
	out.sort_by_format(cfgInnerSort, 0);
	return c;
}

bool DbAlbumCollection::getAlbumForTrack(const metadb_handle_ptr& track, CollectionPos& out){
	t_size mapIdx;
	if(ptrGroupMap.bsearch_t(&(ptrGroupMap_searchPtr), track, mapIdx)){
		out = CollectionPos(this, ptrGroupMap.get_item_ref(mapIdx).groupId);
		return true;
	} else {
		return false;
	}
}


DbAlbumCollection::~DbAlbumCollection(void)
{
	DeleteCriticalSection(&sourceScriptsCS);
}

void DbAlbumCollection::getTitle(CollectionPos pos, pfc::string_base& out){
	ScopeCS scopeLock(renderThreadCS);
	if (albums.get_count() > 0)
		albums.get_item_ref(pos.toIndex())->format_title(0, out, cfgAlbumTitleScript, 0);
	else
		out = "No Covers Loaded";
}

ImgTexture* DbAlbumCollection::getImgTexture(CollectionPos pos){
	if (albums.get_count() < 1)
		return 0;

	pfc::string8_fast_aggressive imgFile;
	imgFile.prealloc(512);
	if (getImageForTrack(albums.get_item_ref(pos.toIndex()), imgFile)){
		return new ImgTexture(imgFile);
	} else {
		return 0;
	}
}

bool DbAlbumCollection::searchAlbumByTitle(const char * title, t_size& o_min, t_size& o_max, CollectionPos& out){
	bool found = false;
	t_size max;
	t_size min;
	if (o_max > titleGroupMap.get_count()){
		max = titleGroupMap.get_count();
	} else {
		max = o_max;
	}
	min = o_min;
	t_size ptr;
	while(min<max)
	{
		ptr = min + ( (max-min) >> 1);
		int result = stricmp_utf8_partial(titleGroupMap[ptr].title, title);
		if (result<0){
			min = ptr + 1;
		} else if (result>0) {
			max = ptr;
		} else { // find borders of area in which stricmp_utf8_partial matches
			t_size iPtr;
			t_size iMin;
			t_size iMax;

			iMin = min;
			iMax = ptr;
			while (iMin < iMax){
				iPtr = iMin + ( (iMax-iMin) >> 1);
				int res = stricmp_utf8_partial(titleGroupMap[iPtr].title, title);
				if (res < 0)
					iMin = iPtr+1;
				if (res == 0)
					iMax = iPtr;
			}
			o_min = iMin;
			iMin = ptr;
			iMax = max;
			while (iMin < iMax){
				iPtr = iMin + ( (iMax-iMin) >> 1);
				int res = stricmp_utf8_partial(titleGroupMap[iPtr].title, title);
				if (res == 0)
					iMin = iPtr+1;
				if (res > 0)
					iMax = iPtr;
			}
			o_max = iPtr+1;

			found = true;
			break;
		}
	}
	if (!found)
		return false;
	int outIdx = titleGroupMap[o_min].groupId;
	{ // find the item with the lowest groupId (this is important to select the leftmost album)
		for(t_size p=o_min; p < o_max; p++){
			if (titleGroupMap[p].groupId < outIdx)
				outIdx = titleGroupMap[p].groupId;
		}
	}
	out = CollectionPos(this, outIdx);
	return true;
}
