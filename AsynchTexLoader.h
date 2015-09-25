#pragma once
#include "stdafx.h"

#include "DbAlbumCollection.h"
#include "Helpers.h"
#include "BlockingQueue.h"

using namespace boost::multi_index;

class AppInstance;
class ImgTexture;

class ATMessage {
public:
	virtual ~ATMessage(){};
};

class ATCollectionReloadMessage : public ATMessage {
	bool threadHalting;
	std::condition_variable cv;
	std::mutex mtx;
public:
	ATCollectionReloadMessage() : threadHalting(false) {};
	void waitForHalt(){
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [this]() {return this->threadHalting; });
	}
	void haltThread(){
		std::unique_lock<std::mutex> lock(mtx);
		threadHalting = true;
		cv.notify_one();
		cv.wait(lock, [this]() {return !this->threadHalting; });
	}
	void allowThreadResume(){
		std::unique_lock<std::mutex> lock(mtx);
		threadHalting = false;
		cv.notify_one();
	}
};

class ATTargetChangedMessage : public ATMessage {};
class ATReloadSpecialTexMessage : public ATMessage {};
class ATPauseMessage : public ATMessage {};
class ATResumeMessage : public ATMessage {};
class ATClearCacheMessage : public ATMessage {};

class ATRenderingDoneMessage : public ATMessage {};

class ATStopThreadMessage : public ATMessage {};


class AsynchTexLoader
{
	AppInstance* appInstance;

public:
	AsynchTexLoader(AppInstance* instance);
	~AsynchTexLoader(void);

	void start();
	void send(shared_ptr<ATMessage> msg);
	shared_ptr<ImgTexture> getLoadedImgTexture(CollectionPos pos);

	// only call this while you hold openglAccess
	void trimCache();
	std::mutex openglAccess;

private:
	void loadSpecialTextures();
	void createLoaderWindow();
	void destroyLoaderWindow();
	void initGlContext();
	HWND glWindow = nullptr;
	HDC glDC = nullptr;
	HGLRC glRC = nullptr;

	shared_ptr<ImgTexture> noCoverTexture;
	shared_ptr<ImgTexture> loadingTexture;

private:
	bool allLoaded = true;
	bool isPaused = false;
	bool loadNextTexture();

	CollectionPos queueCenter;


	void threadProc();
	shared_ptr<ImgTexture> loadTexImage(CollectionPos pos);
	HANDLE workerThread = nullptr;

	int workerThreadPrio;
	void setWorkerThreadPrio(int nPrio, bool force = false);

	void clearCache();
	void tryGlStuff();

	BlockingQueue<shared_ptr<ATMessage>> messageQueue;

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

	boost::synchronized_value<t_textureCache> textureCache;
};
