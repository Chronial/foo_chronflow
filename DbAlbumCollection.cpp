#include "DbAlbumCollection.h"

#include "EngineThread.h"
#include "config.h"
#include "DbReloadWorker.h"
#include "Engine.h"


DbAlbumCollection::DbAlbumCollection() :
	targetPos(albums.get<1>().end())
{
	static_api_ptr_t<titleformat_compiler> compiler;
	compiler->compile_safe_ex(cfgAlbumTitleScript, cfgAlbumTitle);
}

void DbAlbumCollection::onCollectionReload(DbReloadWorker&& worker){
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
		CollectionPos oldTargetPos = targetPos;
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
}

bool DbAlbumCollection::getTracks(CollectionPos pos, metadb_handle_list& out){
	out = pos->tracks;
	out.sort_by_format(cfgInnerSort, nullptr);
	return true;
}

bool DbAlbumCollection::getAlbumForTrack(const metadb_handle_ptr& track, CollectionPos& out){
	pfc::string8_fast_aggressive albumKey;
	if (!albumMapper.is_valid())
		return false;
	track->format_title(0, albumKey, albumMapper, 0);
	if (albums.count(albumKey.get_ptr())){
		auto groupAlbum = albums.find(albumKey.get_ptr());
		out = albums.project<1>(groupAlbum);
		return true; 
	} else {
		return false;
	}
}

AlbumInfo DbAlbumCollection::getAlbumInfo(CollectionPos pos)
{
	metadb_handle_list tracks;
	pfc::string8 title;
	getTracks(pos, tracks);
	getTitle(pos, title);
	return AlbumInfo{title, pos->groupString, tracks};
}

void DbAlbumCollection::getTitle(CollectionPos pos, pfc::string_base& out){
	pos->tracks[0]->format_title(0, out, cfgAlbumTitleScript, 0);
}

struct CompIUtf8Partial
{
	bool operator()(pfc::string8 a, const char * b)const{
		return stricmp_utf8_partial(a, b) < 0;
	}
	bool operator()(const char * a, pfc::string8 b)const{
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
		t_size outIdx = ~0u;
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
	targetPos = newTarget;
	sessionSelectedCover = this->rank(newTarget);
}

void DbAlbumCollection::setTargetByName(const std::string& groupString){
	auto groupAlbum = albums.find(groupString);
	if (groupAlbum != albums.end()) {
		targetPos = albums.project<1>(groupAlbum);
		sessionSelectedCover = this->rank(targetPos);
	}
}

void DbAlbumCollection::moveTargetBy(int n)
{
	movePosBy(targetPos, n);
	sessionSelectedCover = this->rank(targetPos);
}
