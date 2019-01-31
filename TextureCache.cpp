#include "stdafx.h"
#include <process.h>
#include "config.h"
#include "Helpers.h"

#include "TextureCache.h"

#include "AppInstance.h"
#include "Console.h"
#include "DbAlbumCollection.h"
#include "RenderThread.h"
#include "Image.h"


void TextureCache::loadSpecialTextures(){
	loadingTexture = loadSpecialArt(IDR_COVER_LOADING, cfgImgLoading).upload();
	noCoverTexture = loadSpecialArt(IDR_COVER_NO_IMG, cfgImgNoCover).upload();
}

const GLTexture& TextureCache::getLoadedImgTexture(const std::string& albumName)
{
	auto entry = textureCache.find(albumName);
	if (entry != textureCache.end()){
		if (entry->texture)
			return entry->texture.value();
		else
			return noCoverTexture.value();
	} else {
		return loadingTexture.value();
	}
	return loadingTexture.value();
}


void TextureCache::init(){
	loadSpecialTextures();
}

void TextureCache::onTargetChange(){
	collection_read_lock lock(appInstance);
	cacheGeneration += 1;
	// wrap around
	if (cacheGeneration > std::numeric_limits<unsigned int>::max() - 100){
		cacheGeneration = 1;
		for (auto it = textureCache.begin(); it != textureCache.end(); ++it){
			textureCache.modify(it, [&](CacheItem& x) {
				x.priority.first = 0;
			});
		}
	}
	updateLoadingQueue(appInstance->albumCollection->getTargetPos());
}

void TextureCache::onCollectionReload(){
	collectionVersion += 1;
	loadSpecialTextures();
	{
		collection_read_lock lock(appInstance);
		updateLoadingQueue(appInstance->albumCollection->getTargetPos());
	}
}

void TextureCache::trimCache(){
	while (textureCache.size() > (size_t)cfgTextureCacheSize){
		auto& prorityIndex = textureCache.get<1>();
		auto oldestEntry = prorityIndex.begin();
		prorityIndex.erase(oldestEntry);
	}
}


void TextureCache::clearCache(){
	textureCache.clear();
	bgLoader.flushQueue();
}

void TextureCache::uploadTextures(){
	bool redraw = false;
	while (auto loaded = bgLoader.getLoaded()){
		redraw = true;
		auto existing = textureCache.find(loaded->meta.groupString);
		if (existing != textureCache.end()) {
			textureCache.erase(existing);
		}
		std::optional<GLTexture> texture{};
		if (loaded->image)
			texture = loaded->image->upload();
		textureCache.emplace(loaded->meta, std::move(texture));
	}
	if (redraw){
		IF_DEBUG(Console::println(L"Refresh MainWin."));
		appInstance->redrawMainWin();
	}
}

void TextureCache::updateLoadingQueue(const CollectionPos& queueCenter){
	collection_read_lock lock(appInstance);

	bgLoader.flushQueue();

	size_t collectionSize = appInstance->albumCollection->getCount();
	size_t maxLoad = min(collectionSize, size_t(cfgTextureCacheSize*0.8));

	CollectionPos leftLoaded = queueCenter;
	CollectionPos rightLoaded = queueCenter;
	CollectionPos loadNext = queueCenter;

	for (size_t i = 0; i < maxLoad; i++){
		auto cacheEntry = textureCache.find(loadNext->groupString);
		auto priority = std::make_pair(cacheGeneration, -(int)i);
		if (cacheEntry != textureCache.end() && cacheEntry->collectionVersion == collectionVersion){
			textureCache.modify(cacheEntry, [=](CacheItem &x) {
				x.priority = priority;
			});
		} else {
			bgLoader.enqueue(
				loadNext->tracks,
				TextureCacheMeta{loadNext->groupString, collectionVersion, priority});
		}

		if ((i % 2 || leftLoaded == appInstance->albumCollection->begin()) &&
				++CollectionPos(rightLoaded) != appInstance->albumCollection->end()){
			++rightLoaded;
			loadNext = rightLoaded;
			ASSERT(loadNext != appInstance->albumCollection->end());
		} else {
			--leftLoaded;
			loadNext = leftLoaded;
		}
	}
}


TextureLoadingThreads::TextureLoadingThreads() {
	unsigned int threadCount = std::thread::hardware_concurrency();
	for (unsigned int i = 0; i < threadCount; i++){
		threads.push_back(std::thread(&TextureLoadingThreads::run, this));
		SetThreadPriority(threads.back().native_handle(), cfgTexLoaderPrio);
	}
}

TextureLoadingThreads::~TextureLoadingThreads() {
	shouldStop = true;
	for (auto& thread : threads){
		// TODO: This is hacky
		inQueue.push(LoadRequest{});
	}
	for (auto& thread : threads){
		if (thread.joinable()){
			thread.join();
		}
	}
}

void TextureLoadingThreads::run(){
	while (!shouldStop) {
		auto request = inQueue.pop();
		if (shouldStop) break;
		auto art = loadAlbumArt(request.tracks);
		if (shouldStop) break;
		outQueue.push(LoadResponse{
			request.meta, std::move(art)});
	}
}

std::optional<TextureLoadingThreads::LoadResponse> TextureLoadingThreads::getLoaded() {
	return outQueue.popMaybe();
}

void TextureLoadingThreads::flushQueue() {
	inQueue.clear();
}

void TextureLoadingThreads::enqueue(const metadb_handle_list& tracks, const TextureCacheMeta& meta){
	inQueue.push(LoadRequest{meta, tracks});
}
