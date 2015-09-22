#pragma once
#include "stdafx.h"

#include "DbAlbumCollection.h"
#include "Helpers.h"

using namespace boost::multi_index;

class AppInstance;
class ImgTexture;

class AsynchTexLoader
{
	AppInstance* appInstance;

public:
	AsynchTexLoader(AppInstance* instance);
	~AsynchTexLoader(void);

	void setQueueCenter(CollectionPos center);

	// If you wan't to call this before any hardrefresh,
	// you'll have to call loadSpecialTextures() in the constructor
	shared_ptr<ImgTexture> getLoadedImgTexture(CollectionPos pos);


	void pauseLoading();
	void resumeLoading();
	void loadSpecialTextures();

	 //may only be called from Main Thread, while worker is paused
	void clearCache();

	void allowUpload();
	void blockUpload();

private:
	void runGlDelete();
	void createLoaderWindow();
	void destroyLoaderWindow();
	bool initGlContext();
	HWND glWindow;
	HDC glDC;
	HGLRC glRC;

	shared_ptr<ImgTexture> noCoverTexture;
	shared_ptr<ImgTexture> loadingTexture;

	void startWorkerThread();
	static unsigned int WINAPI runWorkerThread(void* lpParameter);
public:
	void stopWorkerThread();

private:

	void queueGlDelete(ImgTexture* tex);

	CollectionPos queueCenter;

	CriticalSection renderContext;
	void reserveRenderContext();
	void releaseRenderContext();

	void workerThreadProc();
	bool loadTexImage(CollectionPos pos, bool doUpload);
	HANDLE workerThread;
	Event workerThreadHasWork;
	CriticalSection workerThreadInLoop;
	int workerThreadPrio;
	void setWorkerThreadPrio(int nPrio, bool force = false);
	bool closeWorkerThread;
	bool queueCenterMoved;

	Event workerThreadMayRun;

	static const int DELETE_BUFFER_SIZE = 256;
	ImgTexture* deleteBuffer[DELETE_BUFFER_SIZE];
	int deleteBufferIn;
	int deleteBufferOut;
	Event deleteBufferFreed;

	std::deque<int> uploadQueue;
	std::deque<shared_ptr<ImgTexture>> deleteQueue;

	Event mayUpload;
	void doUploadQueue();

	CriticalSection textureCacheCS;
	struct CacheItem {
		int texIdx;
		shared_ptr<ImgTexture> texture;
	};
	typedef multi_index_container <
		CacheItem,
		indexed_by<
			hashed_unique<member<CacheItem, int, &CacheItem::texIdx> >,
			sequenced<>
		>
	> t_textureCache;
	t_textureCache textureCache;
};
