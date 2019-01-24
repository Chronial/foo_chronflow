#include "stdafx.h"
#include <process.h>
#include "config.h"
#include "Helpers.h"

#include "TextureCache.h"

#include "AppInstance.h"
#include "Console.h"
#include "DbAlbumCollection.h"
#include "ImgTexture.h"
#include "RenderThread.h"


TextureCache::TextureCache(AppInstance* instance)
	: appInstance(instance), bgLoader(*instance) {}

void TextureCache::loadSpecialTextures(){
	if (loadingTexture){
		loadingTexture->glDelete();
	}
	pfc::string8 loadingTexPath(cfgImgLoading);
	loadingTexPath.skip_trailing_char(' ');
	if (loadingTexPath.get_length() > 0){
		Helpers::fixPath(loadingTexPath);
		loadingTexture = make_shared<ImgTexture>(loadingTexPath);
	}
	else {
		loadingTexture = make_shared<ImgTexture>(IDR_COVER_LOADING, L"JPG");
	}

	if (noCoverTexture){
		noCoverTexture->glDelete();
	}
	pfc::string8 noCoverTexPath(cfgImgNoCover);
	noCoverTexPath.skip_trailing_char(' ');
	if (noCoverTexPath.get_length() > 0){
		Helpers::fixPath(noCoverTexPath);
		noCoverTexture = make_shared<ImgTexture>(noCoverTexPath);
	}
	else {
		noCoverTexture = make_shared<ImgTexture>(IDR_COVER_NO_IMG, L"JPG");
	}
}

shared_ptr<ImgTexture> TextureCache::getLoadedImgTexture(const DbAlbum& pos)
{
	auto entry = textureCache.find(pos.groupString);
	if (entry != textureCache.end()){
		if (entry->texture)
			return entry->texture;
		else
			return noCoverTexture;
	} else {
		return loadingTexture;
	}
}


void TextureCache::init(){
	loadSpecialTextures();
}

TextureCache::~TextureCache(){
	// TODO: These might be nullpointers
	loadingTexture->glDelete();
	noCoverTexture->glDelete();

	clearCache();
}

void TextureCache::onTargetChange(){
	collection_read_lock lock(appInstance);
	cacheGeneration += 1;
	// wrap around
	if (cacheGeneration > std::numeric_limits<unsigned int>::max() - 100){
		cacheGeneration = 1;
		for (auto it = textureCache.begin(); it != textureCache.end(); ++it){
			textureCache.modify(it, [&](TextureCacheItem& x) {
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
		if (oldestEntry->texture)
			oldestEntry->texture->glDelete();
		prorityIndex.erase(oldestEntry);
	}
}


void TextureCache::clearCache(){
	for (auto &e : textureCache){
		if (e.texture)
			e.texture->glDelete();
	}
	textureCache.clear();
	bgLoader.flushQueue();
}

void TextureCache::uploadTextures(){
	bool redraw = false;
	while (boost::optional<TextureCacheItem> item = bgLoader.getLoaded()){
		redraw = true;
		auto existing = textureCache.find(item.get().groupString);
		if (existing != textureCache.end()) {
			if (existing->texture)
				existing->texture->glDelete();
			textureCache.erase(existing);
		}
		if (item->texture)
			item->texture->glUpload();
		textureCache.insert(std::move(item.get()));
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
			textureCache.modify(cacheEntry, [=](TextureCacheItem &x) {
				x.priority = priority;
			});
		} else {
			bgLoader.enqueue(TextureCacheItem{
				loadNext->groupString, collectionVersion, priority, nullptr });
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

TextureLoadingThreads::TextureLoadingThreads(AppInstance& appInstance) :
	appInstance(appInstance)
{
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
		inQueue.push(TextureCacheItem{});
	}
	for (auto& thread : threads){
		if (thread.joinable()){
			thread.join();
		}
	}
}

void TextureLoadingThreads::run(){
	while (!shouldStop) {
		auto item = inQueue.pop();
		if (shouldStop) break;
		item.texture = loadImage(item.groupString);
		if (shouldStop) break;
		outQueue.push(std::move(item));
	}
}


shared_ptr<ImgTexture> TextureLoadingThreads::loadImage(const std::string& albumName){
	collection_read_lock lock(appInstance);
	IF_DEBUG(double preLoad = Helpers::getHighresTimer());
	shared_ptr<ImgTexture> tex = appInstance.albumCollection->getImgTexture(albumName);
#ifdef _DEBUG
	if (tex){
		Console::printf(L"Load image file in %d ms\n", int((Helpers::getHighresTimer() - preLoad) * 1000));
	} else {
		Console::printf(L"Detect missing file in %d ms\n", int((Helpers::getHighresTimer() - preLoad) * 1000));
	}
#endif
	return tex;
}


boost::optional<TextureCacheItem> TextureLoadingThreads::getLoaded() {
	return outQueue.popMaybe();
}

void TextureLoadingThreads::flushQueue() {
	inQueue.clear();
}

void TextureLoadingThreads::enqueue(TextureCacheItem item){
	inQueue.push(item);
}
