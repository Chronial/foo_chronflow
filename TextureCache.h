#pragma once
#include <utility>

#include "BlockingQueue.h"
#include "DbAlbumCollection.h"
#include "Image.h"
#include "utils.h"

namespace bomi = boost::multi_index;

class ImgTexture;
class EngineThread;

struct TextureCacheMeta {
  std::string groupString;
  unsigned int collectionVersion{0};
  // (generation, -distance to center)
  std::pair<unsigned int, int> priority;
};

class TextureLoadingThreads {
 public:
  TextureLoadingThreads();
  NO_MOVE_NO_COPY(TextureLoadingThreads);
  ~TextureLoadingThreads();

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
  std::pair<std::string, metadb_handle_ptr> takeJob();
  void finishJob(const std::string&, std::optional<UploadReadyImage>);

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
  DbAlbumCollection& db;
  EngineThread& thread;
  class ScriptedCoverPositions& coverPos;

 public:
  TextureCache(EngineThread&, DbAlbumCollection&, ScriptedCoverPositions&);

  const GLImage* getAlbumTexture(const std::string& albumName);
  GLImage& getLoadingTexture();

  void trimCache();
  void clearCache();
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

  friend class TextureLoadingThreads;
};
