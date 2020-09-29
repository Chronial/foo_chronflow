#pragma once
// clang-format off
#include <utility>
#include "cover_positions.h"
#include "DbAlbumCollection.h"
#include "EngineThread.fwd.h" //optional fwd
//#include "EngineThread.h"
#include "Image.h"
#include "utils.h"
// clang-format on
namespace render {
using db::DbAlbumCollection;
using db::DBIter;
using db::DBPos;
using engine::EngineThread;

namespace bomi = boost::multi_index;

struct TextureCacheMeta {
  std::string groupString;
  unsigned int coverart{0};
  unsigned int collectionVersion{0};
  // (generation, -distance to center)
  std::pair<unsigned int, int> priority;
};

class TextureLoadingThreads {
 public:
  TextureLoadingThreads() {
    unsigned int threadCount = std::thread::hardware_concurrency();
    for (unsigned int i = 0; i < threadCount; i++) {
      threads.emplace_back(catchThreadExceptions("TextureLoader", [&] { this->run(); }));
      check(SetThreadPriority(threads.back().native_handle(),
                              THREAD_PRIORITY_BELOW_NORMAL));
      // Disable dynamic priority boost. We don't want the texture loaders to ever have
      // higher priority than the engine thread.
      check(SetThreadPriorityBoost(threads.back().native_handle(), TRUE));
    }
    setPriority(true);
  }

  NO_MOVE_NO_COPY(TextureLoadingThreads);
  ~TextureLoadingThreads();

  metadb_handle_ptr tltmetatarget;

  struct LoadRequest {
    TextureCacheMeta meta;
    metadb_handle_ptr track;
  };

  struct LoadResponse {
    TextureCacheMeta meta;
    std::optional<UploadReadyImage> image;
  };

  void flushQueue();
  void setQueue(std::vector<LoadRequest>&& data);
  std::optional<LoadResponse> getLoaded();
  void pause();
  void resume();
  void setPriority(bool highPriority);

 private:
  void takeJob(LoadRequest& loadrequest);
  //std::pair<std::string, metadb_handle_ptr> takeJob();
  void finishJob(const std::string&, std::optional<UploadReadyImage>);

  std::pair<std::string, metadb_handle_ptr> getTarget();

  std::vector<std::thread> threads;
  abort_callback_impl abort;
  std::atomic<bool> highPriority = false;
  std::shared_mutex pauseMutex;
  std::unique_lock<std::shared_mutex> pauseLock{pauseMutex, std::defer_lock};

  std::mutex mutex;
  std::condition_variable inCondition;
  std::deque<LoadRequest> inQueue;
  std::unordered_map<std::string, TextureCacheMeta> inProgress;
  std::deque<LoadResponse> outQueue;

  void run();
};

class TextureCache {
  const DBPos* dbptarget;
  metadb_handle_ptr metatarget;

  DbAlbumCollection& db;
  EngineThread& thread;
  ScriptedCoverPositions& coverPos;

 public:
  TextureCache(EngineThread& thread, DbAlbumCollection& db, ScriptedCoverPositions& coverPos);

  const GLImage* getAlbumTexture(const std::string& albumName);
  GLImage& getLoadingTexture();

  void trimCache();
  void clearCache();
  void modifyCenterCache(bool prev, bool ctrl);
  void startLoading(const DBPos& target);
  void onCollectionReload();
  void updateLoadingQueue(const DBIter& queueCenter);
  void uploadTextures();

  void pauseLoading();
  void resumeLoading();
  void setPriority(bool highPriority);

 private:
  int maxCacheSize();
  unsigned int collectionVersion = 0;

  void reloadSpecialTextures();
  GLImage noCoverTexture;
  GLImage loadingTexture;

  unsigned int cacheGeneration = 0;

  struct CacheItem : TextureCacheMeta {
    CacheItem(const TextureCacheMeta& meta, std::optional<GLImage>&& texture)
        : TextureCacheMeta(meta), texture(std::move(texture)){};
    std::optional<GLImage> texture;
  };
  using t_textureCache = bomi::multi_index_container<
      CacheItem,
      bomi::indexed_by<
          bomi::hashed_unique<
              bomi::member<TextureCacheMeta, std::string, &CacheItem::groupString>>,
          bomi::ordered_non_unique<bomi::composite_key<
              CacheItem,
              bomi::member<TextureCacheMeta, unsigned int, &CacheItem::collectionVersion>,
              bomi::member<TextureCacheMeta, std::pair<unsigned int, int>,
                           &CacheItem::priority>>>>>;

  t_textureCache textureCache;

  TextureLoadingThreads bgLoader;

  friend TextureLoadingThreads;
};
} // namespace render
