#pragma once

typedef int (CALLBACK* t_resynchCallback)(int, void*, AlbumCollection*);


class AsynchTexLoader
{
	AppInstance* appInstance;
public:
	AsynchTexLoader(AppInstance* instance);
public:
	~AsynchTexLoader(void);
public:
	void setQueueCenter(CollectionPos center);
public:
	// If you wan't to call this before any hardrefresh,
	// you'll have to call loadSpecialTextures() in the constructor
	ImgTexture* getLoadedImgTexture(CollectionPos pos);
public:
	void runGlDelete();

public:
	void pauseLoading();
	void resumeLoading();

public:
	void loadSpecialTextures();

public:
	 //may only be called from Main Thread, while worker is paused
	void clearCache();

	// may only be called from Main Thread, while worker is paused
	void resynchCache(t_resynchCallback callback, void* param);
private:
	static void resynchCacheEnumerator(int idx, ImgTexture* tex);

public:
	void allowUpload();
	void blockUpload();

private:
	void createLoaderWindow();
	void destroyLoaderWindow();
	bool initGlContext();
	HWND glWindow;
	HDC glDC;
	HGLRC glRC;

private:
	ImgTexture* noCoverTexture;
	ImgTexture* loadingTexture;
private:
	void startWorkerThread();
	static unsigned int WINAPI runWorkerThread(void* lpParameter);
	void stopWorkerThread();

	void queueGlDelete(ImgTexture* tex);

	CollectionPos queueCenter;

	void workerThreadProc();
	void loadTexImage(CollectionPos pos, bool doUpload);
	HANDLE workerThread;
	Event workerThreadHasWork;
	CriticalSection workerThreadInLoop;
	int workerThreadPrio;
	void setWorkerThreadPrio(int nPrio, bool force = false);
	bool closeWorkerThread;
	int loadDistance;
	CollectionPos loadCenter;

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
	typedef pfc::map_t<int, ImgTexture*> t_textureCache;
	t_textureCache textureCache;
	static void textureCacheDeleteEnumerator(int idx, ImgTexture* tex);

public:
	typedef struct {
		int cacheIdx;
		int distance;
	} t_cleanUpCacheDistance;
private:
	void cleanUpCache();
	static void cleanUpCacheGetDistances(int idx, ImgTexture* tex);
	pfc::list_t<t_cleanUpCacheDistance> cleanUpCacheDistances;
	class cleanUpCacheDistances_compare : public pfc::list_base_t<t_cleanUpCacheDistance>::sort_callback {
	public:
		int compare(const t_cleanUpCacheDistance &a, const t_cleanUpCacheDistance &b){
			return b.distance - a.distance;
		}
	};

private:
	struct EnumHelper;
	static EnumHelper enumHelper;
};
