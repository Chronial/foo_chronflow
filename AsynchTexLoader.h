#pragma once

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

private:
	AlbumCollection* collection;

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

	CRITICAL_SECTION coverTexMapCS;
	int* coverTexMap;
	int textureBufferSize;
	ImgTexture* defaultTexture;
	ImgTexture** textures;
	CollectionPos** texturePos;
};
