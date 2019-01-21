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


TextureCache::TextureCache(AppInstance* instance) :
appInstance(instance)
{
}

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

shared_ptr<ImgTexture> TextureCache::getLoadedImgTexture(CollectionPos pos)
{
	shared_ptr<ImgTexture> tex;
	auto entry = textureCache.find(pos->groupString);
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
	loadingTexture->glDelete();
	noCoverTexture->glDelete();
	clearCache();
}

void TextureCache::onTargetChange(){
	collection_read_lock lock(appInstance);
	cacheGeneration++;
	// wrap around
	if (cacheGeneration > std::numeric_limits<unsigned int>::max() - 100){
		cacheGeneration = 1;
		for (auto it = textureCache.begin(); it != textureCache.end(); ++it){
			textureCache.modify(it, [&](CacheItem& x) {
				x.priority.first = 0;
			});
		}
	}
	queueCenter = appInstance->albumCollection->getTargetPos();
	allLoaded = false;
}

void TextureCache::onCollectionReload(){
	loadSpecialTextures();
	clearCache();
	{
		collection_read_lock lock(appInstance);
		queueCenter = appInstance->albumCollection->getTargetPos();
	}
	allLoaded = false;
}

void TextureCache::clearCache(){
	for (auto &e : textureCache){
		if (e.texture){
			e.texture->glDelete();
		}
	}
	textureCache.clear();
}

void TextureCache::tryGlStuff(){
	TRACK_CALL_TEXT("TextureCache::tryGlStuff");
	while (textureCache.size() > (size_t)cfgTextureCacheSize){
		auto &prorityIndex = textureCache.get<1>();
		auto deleteTexture = prorityIndex.begin()->texture;
		prorityIndex.erase(prorityIndex.begin());
		if (deleteTexture)
			deleteTexture->glDelete();
	}
	auto &uploadedIndex = textureCache.get<2>();
	while (int c = uploadedIndex.count(false)){
		auto toUpload = uploadedIndex.find(false);
		if (toUpload->texture)
			toUpload->texture->glUpload();
		uploadedIndex.modify_key(toUpload, [](bool &isUploaded) {isUploaded = true; });
	}
}

bool TextureCache::loadNextTexture(){
	collection_read_lock lock(appInstance);

	int collectionSize = appInstance->albumCollection->getCount();
	if (collectionSize == 0){
		allLoaded = true;
		return false;
	}
	unsigned int maxLoad = min(collectionSize, int(cfgTextureCacheSize*0.8));
	unsigned int loaded = 0;

	CollectionPos leftLoaded = queueCenter;
	CollectionPos rightLoaded = queueCenter;
	CollectionPos loadNext = queueCenter;

	{
		while(textureCache.count(loadNext->groupString)){
			// make sure the cache contains the right distances
			auto cacheEntry = textureCache.find(loadNext->groupString);
			if (cacheEntry->priority.first != cacheGeneration){
				textureCache.modify(cacheEntry, [=](CacheItem &x) {
					x.priority = std::make_pair(cacheGeneration, -(int)loaded);
				});
			}

			if (++loaded >= maxLoad){
				allLoaded = true;
				return false;
			}

			if ((loaded % 2 || leftLoaded == appInstance->albumCollection->begin()) &&
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

	auto texture = loadTexImage(loadNext);

	// update TextureCache
	textureCache.insert({ loadNext->groupString, { cacheGeneration, -(int)loaded }, texture, false });

	bool nearCenter = loaded < 16;

	if ((nearCenter && (loaded % 2)) ||
		!(loaded % 8)){
		IF_DEBUG(Console::println(L"Refresh MainWin."));
		appInstance->redrawMainWin();
	}
	return true;
}

shared_ptr<ImgTexture> TextureCache::loadTexImage(CollectionPos pos){
	IF_DEBUG(double preLoad = Helpers::getHighresTimer());
	shared_ptr<ImgTexture> tex = appInstance->albumCollection->getImgTexture(pos);
#ifdef _DEBUG
	if (tex){
		Console::printf(L"Load image file in %d ms\n", int((Helpers::getHighresTimer() - preLoad) * 1000));
	} else {
		Console::printf(L"Detect missing file in %d ms\n", int((Helpers::getHighresTimer() - preLoad) * 1000));
	}
#endif
	return tex;
}

void TextureCache::trimCache(){
	while (textureCache.size() > (size_t)cfgTextureCacheSize){
		auto &prorityIndex = textureCache.get<1>();

		auto deleteTexture = prorityIndex.begin()->texture;
		prorityIndex.erase(prorityIndex.begin());
		if (deleteTexture)
			deleteTexture->glDelete();
	}
}