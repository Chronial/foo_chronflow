#include "stdafx.h"
#include "config.h"

#include "DbAlbumCollection.h"

#include "AsynchTexLoader.h"
#include "AppInstance.h"
#include "DisplayPosition.h"
#include "ImgTexture.h"

#include <process.h>

class DbAlbumCollection::RefreshWorker {
private:
	metadb_handle_list library;
	DbAlbums albums;
	service_ptr_t<titleformat_object> albumMapper;

private:
	void moveDataToAppInstance(){
		ScopeCS renderScopeLock(appInstance->albumCollection->renderThreadCS);
		ScopeCS texloadScopeLock(appInstance->albumCollection->texloaderThreadCS);
		appInstance->albumCollection->albums = std::move(albums);
		appInstance->albumCollection->albumMapper = std::move(albumMapper);
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

		auto &groupIndex = albums.get<0>();
		actionStart = Helpers::getHighresTimer();
		{
			compiler->compile(albumMapper, cfgGroup);
			pfc::string8_fast_aggressive tmpSortString;
			albums.reserve(library.get_size());
			service_ptr_t<titleformat_object> sortFormatter;
			if (!cfgSortGroup){
				compiler->compile(sortFormatter, cfgSort);
			}
			for (t_size i = 0; i < library.get_size(); i++){
				pfc::string8_fast groupString;
				metadb_handle_ptr track = library.get_item(i);
				track->format_title(0, groupString, albumMapper, 0);
				if (!groupIndex.count(groupString.get_ptr())){
					std::wstring sortString;
					if (cfgSortGroup){
						sortString = pfc::stringcvt::string_wide_from_utf8(groupString);
					} else {
						track->format_title(0, tmpSortString, sortFormatter, 0);
						sortString = pfc::stringcvt::string_wide_from_utf8(tmpSortString);
					}

					pfc::string8_fast findAsYouType;
					track->format_title(0, findAsYouType, cfgAlbumTitleScript, 0);

					metadb_handle_list tracks;
					tracks.add_item(track);
					albums.insert({ groupString.get_ptr(), std::move(sortString), std::move(findAsYouType), std::move(tracks) });
				} else {
					auto album = groupIndex.find(groupString.get_ptr());
					groupIndex.modify(album, [&](DbAlbum &a) { a.tracks.add_item(track); });
				}
			}
		}
		console::printf("foo_chronflow: Generation took %d msec", int((Helpers::getHighresTimer() - actionStart) * 1000));
	}

private:
	AppInstance* appInstance;
	RefreshWorker(AppInstance* instance){
		this->appInstance = instance;
	}
private:
	HANDLE workerThread;
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
	static void reloadAsynchStart(AppInstance* instance){
		instance->texLoader->pauseLoading();
		RefreshWorker* worker = new RefreshWorker(instance);
		worker->init();
		worker->startWorkerThread();
	}
	void reloadAsynchFinish(){
		ScopeCS scopeLock(appInstance->displayPos->accessCS);
		appInstance->albumCollection->reloadSourceScripts();
		appInstance->texLoader->clearCache();
		
		;

		CollectionPos newCenteredPos;
		auto &sortedIndex = albums.get<1>();

		if (appInstance->albumCollection->getCount() == 0){
			if (albums.size() > (t_size)sessionSelectedCover){
				newCenteredPos = sortedIndex.nth(sessionSelectedCover);
			} else {
				newCenteredPos = sortedIndex.begin();
			}
		} else {
			CollectionPos oldCenteredPos = appInstance->displayPos->getCenteredPos();
			newCenteredPos = sortedIndex.begin();

			pfc::string8_fast_aggressive albumKey;
			for (t_size i = 0; i < oldCenteredPos->tracks.get_size(); i++){
				newCenteredPos->tracks[i]->format_title(0, albumKey, albumMapper, 0);
				if (albums.count(albumKey.get_ptr())){
					newCenteredPos = albums.project<1>(albums.find(albumKey.get_ptr()));
					break;
				}
			}
		}


		appInstance->displayPos->hardSetCenteredPos(newCenteredPos);
		appInstance->displayPos->hardSetTarget(newCenteredPos);
		appInstance->texLoader->setQueueCenter(newCenteredPos);
		// smarter target sync:
		/*if (newCenteredIdx >= 0){
			centeredPos += (newCenteredIdx - centeredIdx);
			appInstance->displayPos->hardSetCenteredPos(centeredPos);
			int newTargetIdx = resynchCallback(targetIdx, collection);
			if (newTargetIdx >= 0){
				targetPos += (newTargetIdx - targetIdx);
			} else {
				targetPos += (newCenteredIdx - centeredIdx);
			}

		}*/

		moveDataToAppInstance();
		appInstance->texLoader->resumeLoading();
		appInstance->redrawMainWin();
		delete this;
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

void DbAlbumCollection::reloadAsynchStart(){
	if (isRefreshing)
		return;
	isRefreshing = true;
	double synchStart = Helpers::getHighresTimer();
	RefreshWorker::reloadAsynchStart(appInstance);
	console::printf("Sync start: %d msec (in mainthread)", int((Helpers::getHighresTimer() - synchStart) * 1000));
	appInstance->texLoader->loadSpecialTextures();
}

void DbAlbumCollection::reloadAsynchFinish(LPARAM worker){
	double synchStart = Helpers::getHighresTimer();
	reinterpret_cast<RefreshWorker*>(worker)->reloadAsynchFinish();
	console::printf("Sync finish: %d msec (in mainthread)",int((Helpers::getHighresTimer()-synchStart)*1000));
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
	out = pos->tracks;
	out.sort_by_format(cfgInnerSort, 0);
	return true;
}

bool DbAlbumCollection::getAlbumForTrack(const metadb_handle_ptr& track, CollectionPos& out){
	auto &sortedIndex = albums.get<1>();
	pfc::string8_fast_aggressive albumKey;
	if (!albumMapper.is_valid())
		return false;
	track->format_title(0, albumKey, albumMapper, 0);
	if (albums.count(albumKey.get_ptr())){
		auto groupAlbum = albums.find(albumKey.get_ptr());
		auto sortAlbum = albums.project<1>(groupAlbum);
		int idx = sortedIndex.rank(sortAlbum);
		out = sortAlbum;
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
	auto &sortedIndex = albums.get<1>();
	if (!albums.empty()){
		pos->tracks[0]->format_title(0, out, cfgAlbumTitleScript, 0);
	} else {
		// FIXME this should never get called
		out = "No Covers Loaded";
	}
}

ImgTexture* DbAlbumCollection::getImgTexture(CollectionPos pos){
	if (albums.empty())
		return 0;

	auto &sortedIndex = albums.get<1>();
	pfc::string8_fast_aggressive imgFile;
	imgFile.prealloc(512);
	if (getImageForTrack(pos->tracks[0], imgFile)){
		return new ImgTexture(imgFile);
	} else {
		return 0;
	}
}

struct CompIUtf8Partial
{
	bool operator()(pfc::string8 a, const char * b)const{
		console::printf("checking for %s", b);
		return stricmp_utf8_partial(a, b) < 0;
	}
	bool operator()(const char * a, pfc::string8 b)const{
		console::printf("checking for %s", a);
		return stricmp_utf8_partial(b, a) > 0;
	}
};

bool DbAlbumCollection::performFayt(const char * title, CollectionPos& out){
	auto &faytIndex = albums.get<2>();
	auto range = faytIndex.equal_range(title, CompIUtf8Partial());

	if (range.first == range.second){
		return false;
	} else {
		auto &sortIndex = albums.get<1>();
		t_size outIdx = ~0;
		out = sortIndex.begin();

		// find the item with the lowest index (this is important to select the leftmost album)
		for (auto it = range.first; it != range.second; ++it){
			CollectionPos thisPos = albums.project<1>(it);
			t_size thisIdx = sortIndex.rank(thisPos);
			if (thisIdx < outIdx){
				outIdx = thisIdx;
				out = thisPos;
			}
		}
		return true;
	}
}


CollectionPos DbAlbumCollection::begin() const{
	return albums.get<1>().begin();
}

CollectionPos DbAlbumCollection::end() const{
	return albums.get<1>().end();
}

t_size DbAlbumCollection::rank(CollectionPos p) {
	return albums.get<1>().rank(p);
}