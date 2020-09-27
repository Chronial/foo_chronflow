#pragma once
#include "DbAlbumInfo.h"
#include "utils.h"

namespace db {
namespace db_structure {

namespace bomi = boost::multi_index;

struct CompWLogical {
  bool operator()(const std::wstring& a, const std::wstring& b) const {
    return StrCmpLogicalW(a.c_str(), b.c_str()) < 0;
  }
};

struct key {};
struct sortKey {};

struct Album {
  Album(const char* key, const wchar_t* sortKey, const char* title)
      : key(key), sortKey(sortKey), title(title){};
  // We want to have permanent references to albums for our reversemap
  NO_MOVE_NO_COPY(Album);

  std::string key;
  std::wstring sortKey;
  std::string title;
  mutable metadb_handle_list tracks;
};

using Container = bomi::multi_index_container<
    Album, bomi::indexed_by<
               bomi::hashed_unique<bomi::tag<key>,
                                   bomi::member<Album, std::string, &Album::key>>,
               bomi::ranked_non_unique<bomi::tag<sortKey>,
                                       bomi::member<Album, std::wstring, &Album::sortKey>,
                                       CompWLogical>>>;

class DB {
 public:
  DB(t_uint64 libraryVersion, const std::string& filterQuery,
     const std::string& keyFormat, const std::string& sortFormat,
     const std::string& titleFormat);
  NO_MOVE_NO_COPY(DB);

  Container container;
  Container::index<key>::type& keyIndex;
  Container::index<sortKey>::type& sortIndex;
  std::map<const metadb_handle_ptr, const db_structure::Album&> trackMap;
  t_uint64 libraryVersion;

  search_filter_v2::ptr filter;
  titleformat_object::ptr keyBuilder;
  titleformat_object::ptr sortFormatter;
  titleformat_object::ptr titleFormatter;
};
using DBIter = Container::index<sortKey>::type::iterator;

}  // namespace db_structure

using db_structure::DBIter;
using db_structure::DB;

class DBWriter {
 public:
  explicit DBWriter(db_structure::DB& db) : db(db){};
  NO_MOVE_NO_COPY(DBWriter);
  void add_tracks(metadb_handle_list_cref tracks, abort_callback& abort);
  void remove_tracks(metadb_handle_list_cref tracks);
  void modify_tracks(metadb_handle_list_cref tracks);

 private:
  void add_track(const metadb_handle_ptr& track);
  void remove_track(const metadb_handle_ptr& track);
  void update_album_metadata(const db_structure::Album& album);
  db_structure::DB& db;

  pfc::string8_fast_aggressive keyBuffer;
  pfc::string8_fast_aggressive sortBuffer;
  pfc::string8_fast_aggressive titleBuffer;
  pfc::stringcvt::string_wide_from_utf8_fast sortBufferWide;
};

class DbAlbumCollection {
 public:
  bool initializing() { return !db; }
  bool empty() { return db ? db->container.empty() : true; }
  int size() { return db ? db->container.size() : 0; }

  AlbumInfo getAlbumInfo(DBIter pos);
  void getTracks(DBIter pos, metadb_handle_list& out);
  std::optional<DBPos> getPosForTrack(const metadb_handle_ptr& track);

  template <class T>
  DBPos posFromIter(T iter) const {
    auto i = db->container.project<0>(iter);
    if (i == db->container.get<0>().end()) {
      return {};
    } else {
      return {iter->key, iter->sortKey};
    }
  };
  // Returns a valid (non-end) iterator or nullopt if db is empty
  std::optional<DBIter> iterFromPos(const DBPos&) const;

  // Gets the leftmost album whose title starts with `input`
  std::optional<DBPos> performFayt(const std::string& input);

  void onCollectionReload(std::unique_ptr<db_structure::DB> newDb);

  DBIter begin() const;
  DBIter end() const;
  int difference(DBIter, DBIter);

  DBIter moveIterBy(const DBIter& p, int n) const;
  DBPos movePosBy(const DBPos& p, int n) const {
    auto iter = iterFromPos(p);
    if (!iter)
      return DBPos();
    return posFromIter(moveIterBy(iter.value(), n));
  }

  enum LibraryChangeType { items_added, items_removed, items_modified };
  void handleLibraryChange(t_uint64 version, LibraryChangeType type,
                           metadb_handle_list tracks);

 private:
  std::vector<std::tuple<t_uint64, LibraryChangeType, metadb_handle_list>>
      libraryChangeQueue;
  unique_ptr<db_structure::DB> db;
};
} // namespace
