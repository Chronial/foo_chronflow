#pragma once
#include "utils.h"

class DbReloadWorker;

struct DBPos {
  std::string key;
  std::wstring sortKey;
};
inline bool operator==(const DBPos& lhs, const DBPos& rhs) {
  return lhs.key == rhs.key;
}
inline bool operator!=(const DBPos& lhs, const DBPos& rhs) {
  return !(lhs == rhs);
}

struct AlbumInfo {
  std::string title;
  DBPos pos;
  metadb_handle_list tracks;
};

namespace db_structure {
namespace bomi = boost::multi_index;

struct CompWLogical {
  bool operator()(const std::wstring& a, const std::wstring& b) const {
    return StrCmpLogicalW(a.c_str(), b.c_str()) < 0;
  }
};

struct CompIUtf8 {
  bool operator()(const std::string& a, const std::string& b) const {
    return stricmp_utf8(a.c_str(), b.c_str()) < 0;
  }
};

struct key {};
struct sortKey {};
struct title {};

struct Album {
  Album(const char* key, const wchar_t* sortKey, const char* title)
      : key(key), sortKey(sortKey), title(title){};
  std::string key;
  std::wstring sortKey;
  std::string title;
  mutable metadb_handle_list tracks;
};

using DB = bomi::multi_index_container<
    Album, bomi::indexed_by<
               bomi::hashed_unique<bomi::tag<key>,
                                   bomi::member<Album, std::string, &Album::key> >,
               bomi::ranked_non_unique<bomi::tag<sortKey>,
                                       bomi::member<Album, std::wstring, &Album::sortKey>,
                                       CompWLogical>,
               bomi::ordered_non_unique<bomi::tag<title>,
                                        bomi::member<Album, std::string, &Album::title>,
                                        CompIUtf8> > >;
}  // namespace db_structure

using DBIter = db_structure::DB::index<db_structure::sortKey>::type::iterator;

class DbAlbumCollection {
 public:
  DbAlbumCollection();

  inline size_t getCount() { return albums.size(); };
  AlbumInfo getAlbumInfo(const DBPos& pos);
  void getTracks(DBIter pos, metadb_handle_list& out);
  std::optional<DBPos> getPosForTrack(const metadb_handle_ptr& track);

  template <class T>
  DBPos posFromIter(T iter) const {
    return {iter->key, iter->sortKey};
  };
  DBIter iterFromPos(const DBPos&) const;

  // Gets the leftmost album whose title starts with `input`
  std::optional<DBPos> performFayt(const std::string& input);

  void onCollectionReload(DbReloadWorker&& worker);

  DBIter begin() const;
  DBIter end() const;
  int difference(const DBPos&, const DBPos&);

  DBIter moveIterBy(const DBIter& p, int n) const;
  DBPos movePosBy(const DBPos& p, int n) const {
    return posFromIter(moveIterBy(iterFromPos(p), n));
  }

 private:
  /******************************* INTERN DATABASE ***************************/
  db_structure::DB albums;
  db_structure::DB::index<db_structure::key>::type& keyIndex;
  db_structure::DB::index<db_structure::sortKey>::type& sortIndex;
  db_structure::DB::index<db_structure::title>::type& titleIndex;
  service_ptr_t<titleformat_object> keyBuilder;
};
