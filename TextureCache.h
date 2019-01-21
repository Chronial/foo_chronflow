#pragma once
#include "stdafx.h"

#include "DbAlbumCollection.h"
#include "Helpers.h"
#include "BlockingQueue.h"

using namespace boost::multi_index;

class AppInstance;
class ImgTexture;

class TextureCache
{
	AppInstance* appInstance;

public:
	TextureCache(AppInstance* instance);
	~TextureCache();

	shared_ptr<ImgTexture> getLoadedImgTexture(CollectionPos pos);

	void trimCache();
	void clearCache();
	void onTargetChange();
	void onCollectionReload();
	bool loadNextTexture();
	void tryGlStuff();

	void init();


private:
	GLFWwindow* glfwWindow = nullptr;
	bool allLoaded = true;
	CollectionPos queueCenter;

	void loadSpecialTextures();
	shared_ptr<ImgTexture> noCoverTexture;
	shared_ptr<ImgTexture> loadingTexture;


	void createLoaderWindow();
	shared_ptr<ImgTexture> loadTexImage(CollectionPos pos);

	unsigned int cacheGeneration = 0;
	struct CacheItem {
		std::string groupString;
		// generation, -distance to center
		std::pair<unsigned int, int> priority;
		shared_ptr<ImgTexture> texture;
		bool isUploaded;
	};
	typedef multi_index_container <
		CacheItem,
		indexed_by<
			hashed_unique<member<CacheItem, std::string, &CacheItem::groupString>>,
			ordered_non_unique<member<CacheItem, std::pair<unsigned int, int>, &CacheItem::priority>>,
			hashed_non_unique<member<CacheItem, bool, &CacheItem::isUploaded>>
		>
	> t_textureCache;

	t_textureCache textureCache;
};
