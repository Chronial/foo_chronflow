#include "DbAlbumCollection.h"

#include "DbReloadWorker.h"
#include "Engine.h"
#include "EngineThread.h"
#include "FindAsYouType.h"
//#include "pfc/range_based_for.h"

namespace db {
using namespace coverflow;
using FuzzyMatcher = engine::FuzzyMatcher;

namespace db_structure {

DB::DB(t_uint64 libraryVersion, const std::string& filterQuery,
       const std::string& keyFormat, const std::string& sortFormat,
       const std::string& titleFormat)
    : keyIndex(container.get<key>()), sortIndex(container.get<sortKey>()),
      libraryVersion(libraryVersion) {
  if (!filterQuery.empty()) {
    try {
      filter ^= search_filter_manager::get()->create(filterQuery.c_str());
    } catch (pfc::exception&) {
    };
  }
  titleformat_compiler::get()->compile_safe_ex(keyBuilder, keyFormat.c_str());
  titleformat_compiler::get()->compile_safe_ex(titleFormatter, titleFormat.c_str());
  if (!sortFormat.empty())
    titleformat_compiler::get()->compile_safe(sortFormatter, sortFormat.c_str());
}

}  // namespace db_structure

void DBWriter::add_tracks(metadb_handle_list_cref tracks, abort_callback& abort) {
  pfc::array_t<bool> filterMask;
  filterMask.set_size(tracks.get_count());
  if (db.filter.is_valid()) {
    db.filter->test_multi_ex(tracks, filterMask.get_ptr(), abort);
  } else {
    filterMask.fill(true);
  }
  abort.check();

  for (t_size i = 0; i < tracks.get_size(); i++) {
    if (!filterMask[i])
      continue;
    abort.check();

    const metadb_handle_ptr& track = tracks[i];
    add_track(track);
  }
}

void DBWriter::modify_tracks(metadb_handle_list_cref tracks) {
  pfc::array_t<bool> filterMask;
  filterMask.set_size(tracks.get_count());
  if (db.filter.is_valid()) {
    db.filter->test_multi(tracks, filterMask.get_ptr());
  } else {
    filterMask.fill(true);
  }
  for (t_size i = 0; i < tracks.get_size(); i++) {
    const metadb_handle_ptr& track = tracks[i];
    bool want = filterMask[i];
    auto kv = db.trackMap.find(track);
    bool didContain = kv != db.trackMap.end();
    if (!didContain && !want) {
      continue;
    } else if (!didContain && want) {
      add_track(track);
    } else if (didContain && !want) {
      remove_track(track);
    } else {  // didContain && want
      auto& album = kv->second;
      track->format_title(nullptr, keyBuffer, db.keyBuilder, nullptr);
      if (album.key != keyBuffer.c_str()) {
        remove_track(track);
        add_track(track);
      } else {
        if (album.tracks.find_item(track) == 0) {
          update_album_metadata(album);
        }
      }
    }
  }
}

void DBWriter::add_track(const metadb_handle_ptr& track) {
  track->format_title(nullptr, keyBuffer, db.keyBuilder, nullptr);
  auto album = db.keyIndex.find(keyBuffer.get_ptr());
  if (album == db.keyIndex.end()) {
    if (db.sortFormatter.is_valid()) {
      track->format_title(nullptr, sortBuffer, db.sortFormatter, nullptr);
      sortBufferWide.convert(sortBuffer);
    } else {
      sortBufferWide.convert(keyBuffer);
    }
    track->format_title(nullptr, titleBuffer, db.titleFormatter, nullptr);
    std::tie(album, std::ignore) =
        db.container.emplace(keyBuffer, sortBufferWide, titleBuffer);
  }
  album->tracks.add_item(track);
  db.trackMap.emplace(track, std::ref(*album));
}

void DBWriter::remove_track(const metadb_handle_ptr& track) {
  auto kv = db.trackMap.find(track);
  if (kv == db.trackMap.end())
    return;
  auto& album = kv->second;
  db.trackMap.erase(kv);
  if (album.tracks.get_size() == 1) {
    db.container.erase(db.container.iterator_to(album));
  } else {
    t_size track_index = album.tracks.find_item(track);
    album.tracks.remove_by_idx(track_index);
    if (track_index == 0) {
      update_album_metadata(album);
    }
  }
}

void DBWriter::update_album_metadata(const db_structure::Album& album) {
  auto& track = album.tracks[0];
  if (db.sortFormatter.is_valid()) {
    track->format_title(nullptr, sortBuffer, db.sortFormatter, nullptr);
    sortBufferWide.convert(sortBuffer);
  } else {
    sortBufferWide.convert(keyBuffer);
  }
  track->format_title(nullptr, titleBuffer, db.titleFormatter, nullptr);
  if (album.sortKey != sortBufferWide.get_ptr() || album.title != titleBuffer.c_str()) {
    PFC_ASSERT(
        db.keyIndex.modify(db.keyIndex.iterator_to(album), [&](db_structure::Album& a) {
          a.sortKey = sortBufferWide;
          a.title = titleBuffer;
        }));
  }
}

void DBWriter::remove_tracks(metadb_handle_list_cref tracks) {
  for (const auto& track : tracks) {
    remove_track(track);
  }
}

void DbAlbumCollection::onCollectionReload(std::unique_ptr<db_structure::DB> newDb) {
  db = std::move(newDb);

  decltype(libraryChangeQueue) changeQueue;
  std::swap(changeQueue, libraryChangeQueue);
  for (auto& change : changeQueue) {
    apply_method(&DbAlbumCollection::handleLibraryChange, *this, std::move(change));
  }
}

void DbAlbumCollection::getTracks(DBIter pos, metadb_handle_list& out) {
  out = pos->tracks;
  out.sort_by_format(cfgInnerSort, nullptr);
}

std::optional<DBPos> DbAlbumCollection::getPosForTrack(const metadb_handle_ptr& track) {
  if (!db)
    return std::nullopt;
  auto kv = db->trackMap.find(track);
  if (kv == db->trackMap.end())
    return std::nullopt;
  return posFromIter(db->keyIndex.iterator_to(kv->second));
}

std::optional<DBIter> DbAlbumCollection::iterFromPos(const DBPos& p) const {
  if (!db || db->container.empty())
    return std::nullopt;
  auto groupItem = db->keyIndex.find(p.key);
  if (groupItem != db->keyIndex.end()) {
    return db->container.project<db_structure::sortKey>(groupItem);
  }
  auto sortItem = db->sortIndex.lower_bound(p.sortKey);
  if (sortItem == db->sortIndex.end()) {
    --sortItem;
  }
  return sortItem;
}

AlbumInfo DbAlbumCollection::getAlbumInfo(DBIter pos) {
  metadb_handle_list tracks;
  getTracks(pos, tracks);
  return AlbumInfo{pos->title, posFromIter(pos), tracks};
}

std::optional<DBPos> DbAlbumCollection::performFayt(const std::string& input) {
  if (!db)
    return std::nullopt;

  FuzzyMatcher matcher(input);

  int maxScore = -1;
  const db_structure::Album* maxAlbum = nullptr;
  for (const auto& album : db->sortIndex) {
    int score = matcher.match(album.title);
    if (score > maxScore) {
      maxScore = score;
      maxAlbum = &album;
    }
  }
  if (maxScore > -1) {
    return posFromIter(db->sortIndex.iterator_to(*maxAlbum));
  } else {
    return std::nullopt;
  }
}

DBIter DbAlbumCollection::begin() const {
  PFC_ASSERT(db);
  return db->sortIndex.begin();
}

DBIter DbAlbumCollection::end() const {
  PFC_ASSERT(db);
  return db->sortIndex.end();
}

int DbAlbumCollection::difference(DBIter a, DBIter b) {
  PFC_ASSERT(db);
  return db->sortIndex.rank(a) - db->sortIndex.rank(b);
}

DBIter DbAlbumCollection::moveIterBy(const DBIter& p, int n) const {
  PFC_ASSERT(db);
  int rank = db->sortIndex.rank(p) + n;
  if (rank <= 0)
    return db->sortIndex.begin();
  if (size_t(rank) >= db->sortIndex.size()) {
    return --db->sortIndex.end();
  }
  return db->sortIndex.nth(rank);
}

void DbAlbumCollection::handleLibraryChange(t_uint64 version, LibraryChangeType type,
                                            metadb_handle_list tracks) {
  if (!db || version > db->libraryVersion) {
    libraryChangeQueue.emplace_back(version, type, std::move(tracks));
    return;
  }
  if (version < db->libraryVersion)
    return;
  DBWriter writer(*db);
  if (type == items_added) {
    abort_callback_dummy aborter{};
    writer.add_tracks(tracks, aborter);
  } else if (type == items_removed) {
    writer.remove_tracks(tracks);
  } else if (type == items_modified) {
    writer.modify_tracks(tracks);
  }
}
}  // namespace db
