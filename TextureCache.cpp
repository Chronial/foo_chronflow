#include "TextureCache.h"

#include "DbAlbumCollection.h"
#include "EngineThread.h"
#include "Image.h"
#include "config.h"
#include "cover_positions.h"
#include "utils.h"

TextureCache::TextureCache(EngineThread& thread, DbAlbumCollection& db,
                           ScriptedCoverPositions& coverPos)
    : db(db), thread(thread), coverPos(coverPos),
      noCoverTexture(loadSpecialArt(IDR_COVER_NO_IMG, cfgImgNoCover.c_str()).upload()),
      loadingTexture(loadSpecialArt(IDR_COVER_LOADING, cfgImgLoading.c_str()).upload()) {}

void TextureCache::reloadSpecialTextures() {
  loadingTexture = loadSpecialArt(IDR_COVER_LOADING, cfgImgLoading.c_str()).upload();
  noCoverTexture = loadSpecialArt(IDR_COVER_NO_IMG, cfgImgNoCover.c_str()).upload();
}

const GLTexture* TextureCache::getAlbumTexture(const std::string& albumName) {
  auto entry = textureCache.find(albumName);
  if (entry == textureCache.end())
    return nullptr;
  if (entry->texture) {
    return &entry->texture.value();
  } else {
    return &noCoverTexture;
  }
}

GLTexture& TextureCache::getLoadingTexture() {
  return loadingTexture;
}

void TextureCache::onTargetChange() {
  cacheGeneration += 1;
  // wrap around
  if (cacheGeneration > std::numeric_limits<unsigned int>::max() - 100) {
    cacheGeneration = 1;
    for (auto it = textureCache.begin(); it != textureCache.end(); ++it) {
      textureCache.modify(it, [&](CacheItem& x) { x.priority.first = 0; });
    }
  }
  updateLoadingQueue(db.getTargetPos());
}

void TextureCache::onCollectionReload() {
  collectionVersion += 1;
  reloadSpecialTextures();
  updateLoadingQueue(db.getTargetPos());
}

int TextureCache::maxCacheSize() {
  int maxDisplay = 1 + std::max(-coverPos.getFirstCover(), coverPos.getLastCover());
  return std::min(int(db.getCount()), std::max(int(cfgTextureCacheSize), 2 * maxDisplay));
}

void TextureCache::trimCache() {
  while (textureCache.size() > static_cast<size_t>(maxCacheSize())) {
    auto& prorityIndex = textureCache.get<1>();
    auto oldestEntry = prorityIndex.begin();
    prorityIndex.erase(oldestEntry);
  }
}

void TextureCache::clearCache() {
  textureCache.clear();
  bgLoader.flushQueue();
}

void TextureCache::uploadTextures() {
  while (auto loaded = bgLoader.getLoaded()) {
    auto existing = textureCache.find(loaded->meta.groupString);
    if (existing != textureCache.end()) {
      textureCache.erase(existing);
    }
    std::optional<GLTexture> texture{};
    if (loaded->image)
      texture = loaded->image->upload();
    textureCache.emplace(loaded->meta, std::move(texture));
  }
}

void TextureCache::updateLoadingQueue(const CollectionPos& queueCenter) {
  bgLoader.flushQueue();

  size_t maxLoad = maxCacheSize();

  CollectionPos leftLoaded = queueCenter;
  CollectionPos rightLoaded = queueCenter;
  CollectionPos loadNext = queueCenter;

  for (size_t i = 0; i < maxLoad; i++) {
    auto cacheEntry = textureCache.find(loadNext->groupString);
    auto priority = std::make_pair(cacheGeneration, -static_cast<int>(i));
    if (cacheEntry != textureCache.end() &&
        cacheEntry->collectionVersion == collectionVersion) {
      textureCache.modify(cacheEntry, [=](CacheItem& x) { x.priority = priority; });
    } else {
      bgLoader.enqueue(loadNext->tracks, TextureCacheMeta{loadNext->groupString,
                                                          collectionVersion, priority});
    }

    if ((((i % 2) != 0u) || leftLoaded == db.begin()) &&
        ++CollectionPos(rightLoaded) != db.end()) {
      ++rightLoaded;
      loadNext = rightLoaded;
      PFC_ASSERT(loadNext != db.end());
    } else {
      --leftLoaded;
      loadNext = leftLoaded;
    }
  }
}

TextureLoadingThreads::TextureLoadingThreads() {
  unsigned int threadCount = std::thread::hardware_concurrency();
  for (unsigned int i = 0; i < threadCount; i++) {
    // NOLINTNEXTLINE(modernize-use-emplace)
    threads.push_back(std::thread(&TextureLoadingThreads::run, this));
    SetThreadPriority(threads.back().native_handle(), cfgTexLoaderPrio);
  }
}

TextureLoadingThreads::~TextureLoadingThreads() {
  shouldStop = true;
#pragma warning(suppress : 4189)
  for ([[maybe_unused]] auto& thread : threads) {
    // TODO: This is hacky
    inQueue.push(LoadRequest{});
  }
  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

void TextureLoadingThreads::run() {
  while (!shouldStop) {
    auto request = inQueue.pop();
    if (shouldStop)
      break;
    auto art = loadAlbumArt(request.tracks);
    if (shouldStop)
      break;
    outQueue.push(LoadResponse{request.meta, std::move(art)});
  }
}

std::optional<TextureLoadingThreads::LoadResponse> TextureLoadingThreads::getLoaded() {
  return outQueue.popMaybe();
}

void TextureLoadingThreads::flushQueue() {
  inQueue.clear();
}

void TextureLoadingThreads::enqueue(const metadb_handle_list& tracks,
                                    const TextureCacheMeta& meta) {
  inQueue.push(LoadRequest{meta, tracks});
}
