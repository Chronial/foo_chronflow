#include "stdafx.h"

#include "DbAlbumCollection.h"

#include "AsynchTexLoader.h"
#include "AppInstance.h"
#include "DisplayPosition.h"
#include "ImgTexture.h"

#include <Shlwapi.h>
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
	std::unordered_map<std::string, DbAlbum> albumMap;
	pfc::list_t<DbAlbum*> sortedAlbums;
	pfc::list_t<DbAlbum*> findAsYouType;
	service_ptr_t<titleformat_object> albumMapper;

private:
	void moveDataToCollection(DbAlbumCollection* collection){
		ScopeCS scopeLock(collection->renderThreadCS);
		sortedAlbums.g_swap(collection->sortedAlbums, sortedAlbums);
		findAsYouType.g_swap(collection->findAsYouType, findAsYouType);
		collection->albumMapper = std::move(albumMapper);
		collection->albumMap = std::move(albumMap);
	}
private:
	// call from mainthread!
	void init(){
		static_api_ptr_t<library_manager> lm;
		lm->get_all_items(library);
	}

	void generateData(){
		static_api_ptr_t<titleformat_compiler> compiler;
		static_api_ptr_t<metadb> db;
		double actionStart;


		actionStart = Helpers::getHighresTimer();
		if (!cfgFilter.is_empty()){
			try {
				search_filter::ptr filter = static_api_ptr_t<search_filter_manager>()->create(cfgFilter);
				pfc::array_t<bool> mask;
				mask.set_size(library.get_count());
				filter->test_multi(library, mask.get_ptr());
				library.filter_mask(mask.get_ptr());
			}
			catch (pfc::exception const &) {};
		}
		console::printf("foo_chronflow: Filter took %d msec", int((Helpers::getHighresTimer() - actionStart) * 1000));


		actionStart = Helpers::getHighresTimer();
		{
			compiler->compile(albumMapper, cfgGroup);
			pfc::string8_fast_aggressive albumKey;
			pfc::string8_fast_aggressive albumSortKey;
			pfc::string8_fast_aggressive albumFAYTKey;
			albumMap.reserve(library.get_size());
			service_ptr_t<titleformat_object> sortFormatter;
			if (!cfgSortGroup){
				compiler->compile(sortFormatter, cfgSort);
			}
			for (t_size i = 0; i < library.get_size(); i++){
				metadb_handle_ptr track = library.get_item(i);
				track->format_title(0, albumKey, albumMapper, 0);
				if (!albumMap.count(albumKey.get_ptr())){
					DbAlbum* album = &albumMap[albumKey.get_ptr()];
					album->tracks.prealloc(20);
					album->tracks.add_item(track);
					if (cfgSortGroup){
						album->sortString = pfc::stringcvt::string_wide_from_utf8(albumKey);
					} else {
						track->format_title(0, albumSortKey, sortFormatter, 0);
						album->sortString = pfc::stringcvt::string_wide_from_utf8(albumSortKey);
					}
					track->format_title(0, album->findAsYouType, cfgAlbumTitleScript, 0);
				} else {
					albumMap[albumKey.get_ptr()].tracks.add_item(track);
				}
			}
		}
		console::printf("foo_chronflow: Grouping took %d msec", int((Helpers::getHighresTimer() - actionStart) * 1000));


		actionStart = Helpers::getHighresTimer();
		for (auto &album : albumMap){
			sortedAlbums.add_item(&(album.second));
		}
		console::printf("foo_chronflow: Full copy took %d msec", int((Helpers::getHighresTimer() - actionStart) * 1000));
		sortedAlbums.sort_t([](DbAlbum* a, DbAlbum* b) {
			return StrCmpLogicalW(a->sortString.data(), b->sortString.data());
		});
		for (size_t i = 0; i < sortedAlbums.get_size(); i++){
			sortedAlbums[i]->index = i;
		}
		console::printf("foo_chronflow: Sorting took %d msec", int((Helpers::getHighresTimer() - actionStart) * 1000));


		actionStart = Helpers::getHighresTimer();
		for (auto &album : albumMap){
			findAsYouType.add_item(&(album.second));
		}
		findAsYouType.sort_stable_t([](DbAlbum* a, DbAlbum* b) {
			return stricmp_utf8(a->findAsYouType, b->findAsYouType);
		});
		console::printf("foo_chronflow: FAYT map generated in %d msec", int((Helpers::getHighresTimer() - actionStart) * 1000));
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
		((RefreshWorker*)(lpParameter))->workerThreadProc();
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

		moveDataToCollection(collection);
		a[4] = Helpers::getHighresTimer();
		appInstance->texLoader->resumeLoading();
		appInstance->redrawMainWin();
		a[5] = Helpers::getHighresTimer();
		for (int i = 0; i < 5; i++){
			console::printf("Times: a%d - a%d: %dmsec", i, i + 1, int((a[i + 1] - a[i]) * 1000));
		}
		delete this;
	}
	static int CALLBACK staticResynchCallback(int oldIdx, void* p_this, AlbumCollection* collection){
		return reinterpret_cast<RefreshWorker*>(p_this)->resynchCallback(oldIdx, collection);
	}
	inline int resynchCallback(int oldIdx, AlbumCollection* collection){
		DbAlbumCollection* col = dynamic_cast<DbAlbumCollection*>(collection);
		if ((size_t)oldIdx >= col->sortedAlbums.get_count())
			return -1;

		DbAlbum* oldAlbum = col->sortedAlbums.get_item_ref(oldIdx);
		int newIdx = -1;
		pfc::string8_fast_aggressive albumKey;
		for (t_size i = 0; i < oldAlbum->tracks.get_size(); i++){
			oldAlbum->tracks[i]->format_title(0, albumKey, albumMapper, 0);
			if (albumMap.count(albumKey.get_ptr())){
				newIdx = albumMap[albumKey.get_ptr()].index;
				break;
			}
		}
		return newIdx;
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
}

void DbAlbumCollection::reloadAsynchStart(bool hardRefresh){
	if (isRefreshing)
		return;
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

bool DbAlbumCollection::getTracks(CollectionPos pos, metadb_handle_list& out){
	size_t idx = pos.toIndex();
	if (idx >= sortedAlbums.get_count()){
		return false;
	}
	out = sortedAlbums[idx]->tracks;
	out.sort_by_format(cfgInnerSort, 0);
	return true;
}

bool DbAlbumCollection::getAlbumForTrack(const metadb_handle_ptr& track, CollectionPos& out){
	pfc::string8_fast_aggressive albumKey;
	if (!albumMapper.is_valid())
		return false;
	track->format_title(0, albumKey, albumMapper, 0);
	try {
		out = CollectionPos(this, albumMap.at(albumKey.get_ptr()).index);
		return true;
	}
	catch (std::out_of_range) {
		return false;
	}
}


DbAlbumCollection::~DbAlbumCollection(void)
{
	DeleteCriticalSection(&sourceScriptsCS);
}

void DbAlbumCollection::getTitle(CollectionPos pos, pfc::string_base& out){
	ScopeCS scopeLock(renderThreadCS);
	if (sortedAlbums.get_count() > 0)
		sortedAlbums[pos.toIndex()]->tracks[0]->format_title(0, out, cfgAlbumTitleScript, 0);
	else
		out = "No Covers Loaded";
}

ImgTexture* DbAlbumCollection::getImgTexture(CollectionPos pos){
	if (albumMap.empty())
		return 0;

	pfc::string8_fast_aggressive imgFile;
	imgFile.prealloc(512);
	if (getImageForTrack(sortedAlbums[pos.toIndex()]->tracks[0], imgFile)){
		return new ImgTexture(imgFile);
	} else {
		return 0;
	}
}

bool DbAlbumCollection::searchAlbumByTitle(const char * title, t_size& o_min, t_size& o_max, CollectionPos& out){
	bool found = false;
	t_size max;
	t_size min;
	if (o_max > findAsYouType.get_count()){
		max = findAsYouType.get_count();
	} else {
		max = o_max;
	}
	min = o_min;
	t_size ptr;
	while(min<max)
	{
		ptr = min + ( (max-min) >> 1);
		int result = stricmp_utf8_partial(findAsYouType[ptr]->findAsYouType, title);
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
				int res = stricmp_utf8_partial(findAsYouType[iPtr]->findAsYouType, title);
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
				int res = stricmp_utf8_partial(findAsYouType[iPtr]->findAsYouType, title);
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
	int outIdx = findAsYouType[o_min]->index;
	{ // find the item with the lowest groupId (this is important to select the leftmost album)
		for(t_size p=o_min; p < o_max; p++){
			if (findAsYouType[p]->index < outIdx)
				outIdx = findAsYouType[p]->index;
		}
	}
	out = CollectionPos(this, outIdx);
	return true;
}
