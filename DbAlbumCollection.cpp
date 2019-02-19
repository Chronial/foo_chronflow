#include "DbAlbumCollection.h"

#include "DbReloadWorker.h"
#include "Engine.h"
#include "EngineThread.h"
#include "config.h"

DbAlbumCollection::DbAlbumCollection()
    : keyIndex(albums.get<db_structure::key>()),
      sortIndex(albums.get<db_structure::sortKey>()),
      titleIndex(albums.get<db_structure::title>()) {
  static_api_ptr_t<titleformat_compiler> compiler;
  compiler->compile_safe_ex(cfgAlbumTitleScript, cfgAlbumTitle);
}

void DbAlbumCollection::onCollectionReload(DbReloadWorker&& worker) {
  albums = std::move(worker.albums);
  keyBuilder = std::move(worker.keyBuilder);
}

void DbAlbumCollection::getTracks(DBIter pos, metadb_handle_list& out) {
  out = pos->tracks;
  out.sort_by_format(cfgInnerSort, nullptr);
}

std::optional<DBPos> DbAlbumCollection::getPosForTrack(const metadb_handle_ptr& track) {
  if (!keyBuilder.is_valid())
    return std::nullopt;
  pfc::string8_fast_aggressive albumKey;
  track->format_title(nullptr, albumKey, keyBuilder, nullptr);
  auto groupAlbum = albums.find(albumKey.get_ptr());
  if (groupAlbum == albums.end())
    return std::nullopt;
  return posFromIter(groupAlbum);
}

DBIter DbAlbumCollection::iterFromPos(const DBPos& p) const {
  if (albums.empty())
    return sortIndex.end();
  auto groupItem = albums.find(p.key);
  if (groupItem != albums.end()) {
    return albums.project<1>(groupItem);
  }
  auto sortItem = sortIndex.lower_bound(p.sortKey);
  if (sortItem == sortIndex.end()) {
    --sortItem;
  }
  return sortItem;
}

AlbumInfo DbAlbumCollection::getAlbumInfo(const DBPos& pos) {
  metadb_handle_list tracks;
  auto iter = iterFromPos(pos);
  getTracks(iter, tracks);
  return AlbumInfo{iter->title, pos, tracks};
}

std::optional<DBPos> DbAlbumCollection::performFayt(const std::string& input) {
  size_t inputLen = pfc::strlen_utf8(input.c_str());
  auto range =
      titleIndex.equal_range(input, [&](const std::string& a, const std::string& b) {
        return stricmp_utf8_partial(a.c_str(), b.c_str(), inputLen) < 0;
      });

  if (range.first == range.second)
    return std::nullopt;

  t_size resRank = ~0u;
  DBIter res;

  // Find the item with the lowest index.
  // This is important to select the leftmost album
  for (auto it = range.first; it != range.second; ++it) {
    DBIter thisPos = albums.project<db_structure::sortKey>(it);
    t_size thisIdx = sortIndex.rank(thisPos);
    if (thisIdx < resRank) {
      resRank = thisIdx;
      res = thisPos;
    }
  }
  return posFromIter(res);
}

DBIter DbAlbumCollection::begin() const {
  return sortIndex.begin();
}

DBIter DbAlbumCollection::end() const {
  return sortIndex.end();
}

int DbAlbumCollection::difference(const DBPos& a, const DBPos& b) {
  return sortIndex.rank(iterFromPos(a)) - sortIndex.rank(iterFromPos(b));
}

DBIter DbAlbumCollection::moveIterBy(const DBIter& p, int n) const {
  auto& sortIndex = albums.get<1>();
  int rank = sortIndex.rank(p) + n;
  if (rank <= 0)
    return sortIndex.begin();
  if (size_t(rank) >= sortIndex.size()) {
    return --sortIndex.end();
  }
  return sortIndex.nth(rank);
}
