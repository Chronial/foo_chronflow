#pragma once
#include "stdafx.h"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>

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
	ImgTexture* getLoadedImgTexture(CollectionPos pos);

	void runGlDelete();

	void pauseLoading();
	void resumeLoading();
	void loadSpecialTextures();

	 //may only be called from Main Thread, while worker is paused
	void clearCache();

	void allowUpload();
	void blockUpload();

private:
	void createLoaderWindow();
	void destroyLoaderWindow();
	bool initGlContext();
	HWND glWindow;
	HDC glDC;
	HGLRC glRC;

	ImgTexture* noCoverTexture;
	ImgTexture* loadingTexture;

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

	struct UploadQueueElem {
		int texIdx;
		UploadQueueElem* next;
	};
	UploadQueueElem* uploadQueueHead;
	UploadQueueElem* uploadQueueTail;
	Event mayUpload;
	void doUploadQueue();

	CriticalSection textureCacheCS;
	struct CacheItem {
		int texIdx;
		ImgTexture* texture;
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
