#pragma once
#include "stdafx.h"

#include "DbAlbumCollection.h"
#include "Helpers.h"
#include "BlockingQueue.h"

using namespace boost::multi_index;

class AppInstance;
class ImgTexture;

struct TextureCacheItem {
	std::string groupString;
	unsigned int collectionVersion;
	// (generation, -distance to center)
	std::pair<unsigned int, int> priority;
	shared_ptr<ImgTexture> texture;
};


class TextureLoadingThreads {
public:
	TextureLoadingThreads(AppInstance& appInstance);
	~TextureLoadingThreads();

	void flushQueue();
	void enqueue(TextureCacheItem item);
	boost::optional<TextureCacheItem> getLoaded();
private:
	AppInstance& appInstance;
	std::vector<std::thread> threads;
	std::atomic<bool> shouldStop = false;

	BlockingQueue<TextureCacheItem> inQueue;
	BlockingQueue<TextureCacheItem> outQueue;

	void run();
	shared_ptr<ImgTexture> loadImage(const std::string& albumName);
};

class TextureCache
{
	AppInstance* appInstance;

public:
	TextureCache(AppInstance* instance);
	~TextureCache();
	void init();

	shared_ptr<ImgTexture> getLoadedImgTexture(const DbAlbum& pos);

	void trimCache();
	void clearCache();
	void onTargetChange();
	void onCollectionReload();
	void TextureCache::updateLoadingQueue(const CollectionPos& queueCenter);
	void uploadTextures();

private:
	GLFWwindow* glfwWindow = nullptr;
	unsigned int collectionVersion = 0;

	void loadSpecialTextures();
	shared_ptr<ImgTexture> noCoverTexture;
	shared_ptr<ImgTexture> loadingTexture;

	unsigned int cacheGeneration = 0;

	typedef multi_index_container <
		TextureCacheItem,
		indexed_by<
			hashed_unique<member<TextureCacheItem, std::string, &TextureCacheItem::groupString>>,
			ordered_non_unique<composite_key<
				TextureCacheItem,
				member<TextureCacheItem, unsigned int, &TextureCacheItem::collectionVersion>,
				member<TextureCacheItem, std::pair<unsigned int, int>, &TextureCacheItem::priority>
			>>
		>
	> t_textureCache;

	t_textureCache textureCache;

	TextureLoadingThreads bgLoader;

	friend class TextureLoadingThreads;
};
