// clang-format off
#include "TextureCache.h"

#include "ConfigData.h"
//#include "EngineThread.h"
#include "utils.h"
// clang-format on
namespace render {

using coverflow::configData;
// namespace bomi = boost::multi_index;

TextureCache::TextureCache(EngineThread& thread, DbAlbumCollection& db,
                           ScriptedCoverPositions& coverPos)
    : db(db), thread(thread), coverPos(coverPos),
      noCoverTexture(loadSpecialArt(IDR_COVER_NO_IMG, cfgImgNoCover.c_str()).upload()),
      loadingTexture(loadSpecialArt(IDR_COVER_LOADING, cfgImgLoading.c_str()).upload()) {}

void TextureCache::reloadSpecialTextures() {
  loadingTexture = loadSpecialArt(IDR_COVER_LOADING, cfgImgLoading.c_str()).upload();
  noCoverTexture = loadSpecialArt(IDR_COVER_NO_IMG, cfgImgNoCover.c_str()).upload();
}

const GLImage* TextureCache::getAlbumTexture(const std::string& albumName) {
  auto entry = textureCache.find(albumName);
  if (entry == textureCache.end())
    return nullptr;
  if (entry->texture) {
    return &entry->texture.value();
  } else {
    return &noCoverTexture;
  }
}

GLImage& TextureCache::getLoadingTexture() {
  return loadingTexture;
}

void TextureCache::startLoading(const DBPos& target) {
  cacheGeneration += 1;
  // wrap around
  if (cacheGeneration > std::numeric_limits<unsigned int>::max() - 100) {
    cacheGeneration = 1;
    for (auto it = textureCache.begin(); it != textureCache.end(); ++it) {
      textureCache.modify(it, [&](CacheItem& x) { x.priority.first = 0; });
    }
  }
  if (auto iter = db.iterFromPos(target)) {
    updateLoadingQueue(iter.value());
  }
}

void TextureCache::onCollectionReload() {
  collectionVersion += 1;
  reloadSpecialTextures();
}

int TextureCache::maxCacheSize() {
  int maxDisplay = 1 + std::max(-coverPos.getFirstCover(), coverPos.getLastCover());
  return std::min(db.size(), std::max(int(cfgTextureCacheSize), 2 * maxDisplay));
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
  glFlush();
  bgLoader.flushQueue();
}

void TextureCache::uploadTextures() {
  while (auto loaded = bgLoader.getLoaded()) {
    auto existing = textureCache.find(loaded->meta.groupString);
    if (existing != textureCache.end()) {
      textureCache.erase(existing);
    }
    std::optional<GLImage> texture{};
    if (loaded->image)
      texture = loaded->image->upload();
    textureCache.emplace(loaded->meta, std::move(texture));
  }
}

void TextureCache::pauseLoading() {
  bgLoader.pause();
}

void TextureCache::resumeLoading() {
  bgLoader.resume();
}

void TextureCache::setPriority(bool highPriority) {
  bgLoader.setPriority(highPriority);
}

void TextureCache::updateLoadingQueue(const DBIter& queueCenter) {
  // Update loaded textures from background loader
  // There is a race here: if a loader finishes loading an image between this call
  // and the call to setQueue below, we might load that image twice.
  uploadTextures();
  size_t maxLoad = maxCacheSize();

  std::vector<TextureLoadingThreads::LoadRequest> requests;

  DBIter leftLoaded = queueCenter;
  DBIter rightLoaded = queueCenter;
  DBIter loadNext = queueCenter;

  for (size_t i = 0; i < maxLoad; i++) {
    auto cacheEntry = textureCache.find(loadNext->key);
    auto priority = std::make_pair(cacheGeneration, -static_cast<int>(i));
    if (cacheEntry != textureCache.end() &&
        cacheEntry->collectionVersion == collectionVersion) {
      textureCache.modify(cacheEntry, [=](CacheItem& x) { x.priority = priority; });
    } else {
      // We only consider one track for art extraction for performance reasons
      requests.emplace_back(TextureLoadingThreads::LoadRequest{
          TextureCacheMeta{loadNext->key, collectionVersion, priority},
          loadNext->tracks[0]});
    }

    if (i == maxLoad - 1)
      break;
    if ((((i % 2) != 0u) || leftLoaded == db.begin()) &&
        ++DBIter(rightLoaded) != db.end()) {
      ++rightLoaded;
      loadNext = rightLoaded;
      PFC_ASSERT(loadNext != db.end());
    } else {
      PFC_ASSERT(leftLoaded != db.begin());
      --leftLoaded;
      loadNext = leftLoaded;
    }
  }

  bgLoader.setQueue(std::move(requests));
}

TextureLoadingThreads::TextureLoadingThreads() {
  unsigned int threadCount = std::thread::hardware_concurrency();
  for (unsigned int i = 0; i < threadCount; i++) {
    threads.emplace_back(catchThreadExceptions("TextureLoader", [&] { this->run(); }));
    check(
        SetThreadPriority(threads.back().native_handle(), THREAD_PRIORITY_BELOW_NORMAL));
    // Disable dynamic priority boost. We don't want the texture loaders to ever have
    // higher priority than the engine thread.
    check(SetThreadPriorityBoost(threads.back().native_handle(), TRUE));
  }
  setPriority(true);
}

TextureLoadingThreads::~TextureLoadingThreads() {
  abort.set();
  resume();
  inCondition.notify_all();
  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

void TextureLoadingThreads::run() {
  bool inBackground = false;
  for (;;) {
    abort.check();
    pauseMutex.lock_shared();
    pauseMutex.unlock_shared();
    abort.check();
    auto [jobId, track] = takeJob();
    abort.check();

    bool shouldBackground = !highPriority.load(std::memory_order_relaxed);
    if (shouldBackground != inBackground) {
      check(SetThreadPriority(GetCurrentThread(), shouldBackground
                                                      ? THREAD_MODE_BACKGROUND_BEGIN
                                                      : THREAD_MODE_BACKGROUND_END));
      inBackground = shouldBackground;
    }

    auto art = loadAlbumArt(track, abort);
    abort.check();
    finishJob(jobId, std::move(art));
  }
}

std::optional<TextureLoadingThreads::LoadResponse> TextureLoadingThreads::getLoaded() {
  std::scoped_lock lock{mutex};
  if (outQueue.empty())
    return std::nullopt;
  auto rc = std::make_optional(std::move(outQueue.back()));
  outQueue.pop_back();
  return rc;
}

void TextureLoadingThreads::pause() {
  if (!pauseLock.owns_lock())
    pauseLock.lock();
}

void TextureLoadingThreads::resume() {
  if (pauseLock.owns_lock())
    pauseLock.unlock();
}

void TextureLoadingThreads::setPriority(bool highPriority) {
  this->highPriority.store(highPriority, std::memory_order_relaxed);
}

void TextureLoadingThreads::flushQueue() {
  std::scoped_lock lock{mutex};
  inQueue.clear();
}

void TextureLoadingThreads::setQueue(std::vector<LoadRequest>&& data) {
  {
    std::scoped_lock lock{mutex};
    inQueue.clear();
    for (auto&& e : data) {
      auto workItem = inProgress.find(e.meta.groupString);
      if (workItem != inProgress.end()) {
        workItem->second = std::move(e.meta);
      } else {
        inQueue.push_back(std::move(e));
      }
    }
  }
  inCondition.notify_all();
}

std::pair<std::string, metadb_handle_ptr> TextureLoadingThreads::takeJob() {
  std::unique_lock lock{mutex};
  inCondition.wait(lock, [&] { return abort.is_aborting() || !inQueue.empty(); });
  abort.check();
  LoadRequest rc(std::move(inQueue.front()));
  inQueue.pop_front();
  inProgress[rc.meta.groupString] = rc.meta;
  return {rc.meta.groupString, rc.track};
}

void TextureLoadingThreads::finishJob(const std::string& id,
                                      std::optional<UploadReadyImage> result) {
  std::unique_lock lock{mutex};
  auto job = inProgress.extract(id);
  outQueue.push_back(LoadResponse{std::move(job.mapped()), std::move(result)});
}
}  // namespace render
