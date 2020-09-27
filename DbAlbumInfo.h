#pragma once
namespace db {

struct DBPos {
  std::string key;
  std::wstring sortKey;
};

inline bool operator==(const DBPos& lhs, const DBPos& rhs) { return lhs.key == rhs.key; }
inline bool operator!=(const DBPos& lhs, const DBPos& rhs) { return !(lhs == rhs); }

struct AlbumInfo {
  std::string title;
  DBPos pos;
  metadb_handle_list tracks;
};

} // namespace db
