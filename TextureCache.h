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
  unsigned int collectionVersion;
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
  void enqueue(const metadb_handle_ptr& track, const TextureCacheMeta& meta);
  std::optional<LoadResponse> getLoaded();
  void pause();
  void resume();
  void setPriority(bool highPriority);

 private:
  std::vector<std::thread> threads;
  std::atomic<bool> shouldStop = false;
  std::atomic<bool> highPriority = false;
  std::shared_mutex pauseMutex;
  std::unique_lock<std::shared_mutex> pauseLock{pauseMutex, std::defer_lock};

  BlockingQueue<LoadRequest> inQueue;
  BlockingQueue<LoadResponse> outQueue;

  void run();
};

class TextureCache {
  DbAlbumCollection& db;
  EngineThread& thread;
  class ScriptedCoverPositions& coverPos;

 public:
  TextureCache(EngineThread&, DbAlbumCollection&, ScriptedCoverPositions&);

  const GLTexture* getAlbumTexture(const std::string& albumName);
  GLTexture& getLoadingTexture();

  void trimCache();
  void clearCache();
  void onTargetChange();
  void onCollectionReload();
  void updateLoadingQueue(const CollectionPos& queueCenter);
  void uploadTextures();

  void pauseLoading();
  void resumeLoading();
  void setPriority(bool highPriority);

 private:
  int maxCacheSize();
  unsigned int collectionVersion = 0;

  void reloadSpecialTextures();
  GLTexture noCoverTexture;
  GLTexture loadingTexture;

  unsigned int cacheGeneration = 0;

  struct CacheItem : TextureCacheMeta {
    CacheItem(const TextureCacheMeta& meta, std::optional<GLTexture>&& texture)
        : TextureCacheMeta(meta), texture(std::move(texture)){};
    std::optional<GLTexture> texture;
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
