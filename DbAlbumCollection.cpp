#include "stdafx.h"
#include "config.h"

#include "DbAlbumCollection.h"

#include "AppInstance.h"
#include "ImgTexture.h"
#include "RenderThread.h"

#include "DbReloadWorker.h"

collection_read_lock::collection_read_lock(AppInstance* appInstance) :
	boost::shared_lock<DbAlbumCollection>(*(appInstance->albumCollection)){};

void DbAlbumCollection::reloadSourceScripts(){
	ASSERT_EXCLUSIVE(this);
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

DbAlbumCollection::DbAlbumCollection(AppInstance* instance):
		targetPos(albums.get<1>().end()){
	this->appInstance = instance;
	InitializeCriticalSectionAndSpinCount(&sourceScriptsCS, 0x80000400);
	isRefreshing = false;

	static_api_ptr_t<titleformat_compiler> compiler;
	compiler->compile(cfgAlbumTitleScript, cfgAlbumTitle);
}

void DbAlbumCollection::startAsyncReload(){
	if (isRefreshing)
		return;
	isRefreshing = true;
	double synchStart = Helpers::getHighresTimer();
	DbReloadWorker::startNew(appInstance);
	console::printf("Sync start: %d msec (in mainthread)", int((Helpers::getHighresTimer() - synchStart) * 1000));
}

void DbAlbumCollection::onCollectionReload(DbReloadWorker& worker){
	ASSERT_EXCLUSIVE(this);
	this->reloadSourceScripts();
	
	// Synchronize TargetPos
	CollectionPos newTargetPos;
	auto &newSortedIndex = worker.albums.get<1>();
	if (albums.size() == 0){
		if (worker.albums.size() > (t_size)sessionSelectedCover){
			newTargetPos = newSortedIndex.nth(sessionSelectedCover);
		} else {
			newTargetPos = newSortedIndex.begin();
		}
	} else {
		newTargetPos = newSortedIndex.begin();
		CollectionPos oldTargetPos = *targetPos;
		pfc::string8_fast_aggressive albumKey;
		for (t_size i = 0; i < oldTargetPos->tracks.get_size(); i++){
			oldTargetPos->tracks[i]->format_title(0, albumKey, worker.albumMapper, 0);
			if (worker.albums.count(albumKey.get_ptr())){
				newTargetPos = worker.albums.project<1>(worker.albums.find(albumKey.get_ptr()));
				break;
			}
		}
	}
	albums = std::move(worker.albums);
	albumMapper = std::move(worker.albumMapper);

	setTargetPos(newTargetPos);

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

shared_ptr<ImgTexture> DbAlbumCollection::getImgTexture(CollectionPos pos){
	if (albums.empty())
		return 0;

	auto &sortedIndex = albums.get<1>();
	pfc::string8_fast_aggressive imgFile;
	imgFile.prealloc(512);
	if (getImageForTrack(pos->tracks[0], imgFile)){
		return make_shared<ImgTexture>(imgFile);
	} else {
		return nullptr;
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




void DbAlbumCollection::setTargetPos(CollectionPos newTarget) {
	ASSERT_SHARED(this);
	*targetPos = newTarget;
	sessionSelectedCover = this->rank(newTarget);
	appInstance->renderer->send(make_shared<RTTargetChangedMessage>());
}

void DbAlbumCollection::moveTargetBy(int n)
{
	auto target = targetPos.synchronize();
	movePosBy(*target, n);
	sessionSelectedCover = this->rank(*target);
	appInstance->renderer->send(make_shared<RTTargetChangedMessage>());
}