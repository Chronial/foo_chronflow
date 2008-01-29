#pragma once

typedef int (CALLBACK* t_resynchCallback)(int, void*, AlbumCollection*);


class AsynchTexLoader
{
public:
	AsynchTexLoader(AlbumCollection* collection);
public:
	~AsynchTexLoader(void);
public:
	void setQueueCenter(CollectionPos center);
	void setNotifyWindow(HWND hWnd);
public:
	ImgTexture* getLoadedImgTexture(CollectionPos pos);
public:
	void runGlDelete();
	bool runGlUpload(unsigned int limit = INFINITE); //returns true if the end of the queue was reached
public:
	void startLoading();
	void stopLoading();

public:
	 //may only be called from Main Thread, while worker is stopped
	void clearCache();

	// may only be called from Main Thread, while worker is stopped
	void resynchCache(t_resynchCallback callback, void* param);
private:
	static void resynchCacheEnumerator(int idx, ImgTexture* tex);


private:
	AlbumCollection* collection;

	ImgTexture* noCoverTexture;
	ImgTexture* loadingTexture;
private:
	void startWorkerThread();
	static unsigned int WINAPI runWorkerThread(void* lpParameter);
	void stopWorkerThread();

	void queueGlUpload(int coverTexMapIdx);
	void queueGlDelete(ImgTexture* tex);
	HWND notifyWindow;

	CollectionPos queueCenter;

	void workerThreadProc();
	void loadTexImage(CollectionPos pos);
	HANDLE workerThread;
	HANDLE workerThreadHasWork;
	int workerThreadPrio;
	void setWorkerThreadPrio(int nPrio, bool force = false);
	bool closeWorkerThread;
	int loadDistance;
	CollectionPos loadCenter;

	static const int DELETE_BUFFER_SIZE = 256;
	ImgTexture* deleteBuffer[DELETE_BUFFER_SIZE];
	int deleteBufferIn;
	int deleteBufferOut;
	HANDLE deleteBufferFreed;

	static const int UPLOAD_QUEUE_SIZE = 128;
	int uploadQueue[UPLOAD_QUEUE_SIZE]; // Indexes of coverTexMap (so we do not upload deleted textures)
	int uploadQueueIn;
	int uploadQueueOut;

	CRITICAL_SECTION textureCacheCS;
	typedef pfc::map_t<int, ImgTexture*> t_textureCache;
	t_textureCache textureCache;
	//int* coverTexMap;
	int textureCacheSize;
	//ImgTexture** textures;
	//CollectionPos** texturePos;
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
