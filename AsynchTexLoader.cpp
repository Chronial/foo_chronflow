#include "chronflow.h"
#include <process.h>

extern cfg_string cfgImgNoCover;
extern cfg_string cfgImgLoading;

struct AsynchTexLoader::EnumHelper {
	CollectionPos* loadCenter;
	AlbumCollection* collection;
	pfc::list_t<AsynchTexLoader::t_cleanUpCacheDistance>* cleanUpCacheDistances;

	t_resynchCallback resynchCallback;
	void* resynchCallbackParam;
	t_textureCache* resynchCallbackOut;
	AlbumCollection* resynchCollection;
};
AsynchTexLoader::EnumHelper AsynchTexLoader::enumHelper = {0};

AsynchTexLoader::AsynchTexLoader(AlbumCollection* collection)
: queueCenter(collection,0), loadCenter(collection, 0)
{
	this->notifyWindow = 0;
	this->collection = collection;
	InitializeCriticalSectionAndSpinCount(&textureCacheCS, 0x80000400);

	textureCacheSize = 100; // this has to be larger than the amount of currently displayed covers!
	//if (textureCacheSize > collection->getCount())
	//	textureCacheSize = collection->getCount();
	
	loadingTexture = new ImgTexture(cfgImgLoading);
	noCoverTexture = new ImgTexture(cfgImgNoCover);

	workerThreadHasWork = CreateEvent(NULL,FALSE,FALSE,NULL);
	workerThread = 0;

	ZeroMemory(deleteBuffer,sizeof(deleteBuffer));
	deleteBufferIn = deleteBufferOut = 0;
	deleteBufferFreed = CreateEvent(NULL,FALSE,FALSE,NULL);

	uploadQueueIn = uploadQueueOut = 0;
	
}

void AsynchTexLoader::startLoading(){
	stopWorkerThread();
	startWorkerThread();
}

void AsynchTexLoader::stopLoading(){
	stopWorkerThread();
}



AsynchTexLoader::~AsynchTexLoader(void)
{
	stopLoading();
	/*for (int i=0; i < textureBufferSize; ++i){
		if (textures[i] && textures[i] != noCoverTexture){
			textures[i]->glDelete();
			delete textures[i];
			delete texturePos[i];
		}
	}*/
	clearCache();
	loadingTexture->glDelete();
	delete loadingTexture;
	noCoverTexture->glDelete();
	delete noCoverTexture;
	CloseHandle(workerThreadHasWork);
	DeleteCriticalSection(&textureCacheCS);
}
void AsynchTexLoader::textureCacheDeleteEnumerator(int idx, ImgTexture* tex){
	if (tex){
		tex->glDelete();
		delete tex;
	}
}

void AsynchTexLoader::setNotifyWindow(HWND hWnd){
	this->notifyWindow = hWnd;
}

ImgTexture* AsynchTexLoader::getLoadedImgTexture(CollectionPos pos)
{
	ImgTexture* tex;
	int idx = pos.toIndex();
#ifdef _DEBUG
	if (!TryEnterCriticalSection(&textureCacheCS)){
		Console::println(L"---------------------------- WAITING for CS -----------------------------");
		EnterCriticalSection(&textureCacheCS);
	}
#else
	EnterCriticalSection(&textureCacheCS);
#endif
	if(textureCache.query(idx, tex)){
		if (tex == 0)
			tex = noCoverTexture;
	} else {
		tex = loadingTexture;
	}
	LeaveCriticalSection(&textureCacheCS);
	return tex;
}

void AsynchTexLoader::setQueueCenter(CollectionPos center)
{
	if (!(center == queueCenter)){
		queueCenter = center;
		SetEvent(workerThreadHasWork);
	}
}

void AsynchTexLoader::setWorkerThreadPrio(int nPrio, bool force)
{
	if (workerThreadPrio != nPrio || force){
		workerThreadPrio = nPrio;
		SetThreadPriority(workerThread,nPrio);
	}
}

void AsynchTexLoader::startWorkerThread()
{
	closeWorkerThread = false;
	workerThread = (HANDLE)_beginthreadex(0,0,&(this->runWorkerThread),(void*)this,0,0);
	setWorkerThreadPrio(THREAD_PRIORITY_NORMAL, true);
}

unsigned int WINAPI AsynchTexLoader::runWorkerThread(void* lpParameter)
{
	((AsynchTexLoader*)(lpParameter))->workerThreadProc();
	return 0;
}

void AsynchTexLoader::stopWorkerThread()
{
	if (workerThread){
		closeWorkerThread = true;
		SetEvent(workerThreadHasWork);
		WaitForSingleObject(workerThread,INFINITE);
		CloseHandle(workerThread);
		workerThread = 0;
	}
}

void AsynchTexLoader::clearCache()
{
	textureCache.enumerate(textureCacheDeleteEnumerator);
	textureCache.remove_all();
}

void AsynchTexLoader::resynchCache(t_resynchCallback callback, void* param){
	t_textureCache oldCache = textureCache;
	textureCache.remove_all();
	enumHelper.resynchCallback = callback;
	enumHelper.resynchCallbackParam = param;
	enumHelper.resynchCallbackOut = &textureCache;
	enumHelper.resynchCollection = collection;
	oldCache.enumerate(resynchCacheEnumerator);
}

void AsynchTexLoader::resynchCacheEnumerator(int idx, ImgTexture* tex){
	int newIdx = enumHelper.resynchCallback(idx, enumHelper.resynchCallbackParam, enumHelper.resynchCollection);
	if (newIdx >= 0)
		enumHelper.resynchCallbackOut->set(newIdx, tex);
	else {
		if (tex){
			tex->glDelete();
			delete tex;
		}
	}
}



void AsynchTexLoader::workerThreadProc()
{
	loadDistance = -1;
	loadCenter = queueCenter;
	bool nearCenter = true;
	//WaitForSingleObject(workerThreadHasWork,INFINITE);
	while(!closeWorkerThread){
		CollectionPos queueCenterLoc = queueCenter; // work with thread-local copy to be Thread-safe
		if (!(loadCenter == queueCenterLoc)){
			int centerMove = queueCenterLoc - loadCenter;
			loadDistance -= abs(centerMove)*2; // not 100% correct
			if (loadDistance < 0)
				loadDistance = -1;
			loadCenter = queueCenterLoc;
		}
		static int cleanUpC = 0;
		cleanUpC++;
		if (loadDistance == int((textureCacheSize - 1)*0.9)){
			cleanUpCache();
			cleanUpC = 0;
			WaitForSingleObject(workerThreadHasWork,INFINITE);
		} else {
			loadDistance++;
			if (loadDistance < 15){
				if (!nearCenter){
					nearCenter = true;
					setWorkerThreadPrio(THREAD_PRIORITY_BELOW_NORMAL);
				}
			} else {
				if (nearCenter){
					nearCenter = false;
					setWorkerThreadPrio(THREAD_PRIORITY_IDLE);
				}
			}
			if (!nearCenter && cleanUpC > 10 && cleanUpC > textureCacheSize * 0.3){
				cleanUpCache();
				cleanUpC = 0;
			}


			CollectionPos loadTarget = loadCenter;
			if (loadDistance % 2){
				loadTarget += (loadDistance + 1) / 2;
			} else {
				loadTarget -= loadDistance / 2;
			}
#ifdef _DEBUG
			Console::printf(L"L d:%2d - idx:%4d\n",loadDistance, loadTarget.toIndex());
#endif
			loadTexImage(loadTarget);
			if (notifyWindow){
				static const int notifyThreshold = 10;
				static int notifyC = 0;
				if ((loadDistance == 0) ||
					(++notifyC == notifyThreshold) ||
					(nearCenter && (notifyC % 2))){
					notifyC = 0;
#ifdef _DEBUG
					Console::println(L"                    UL trig.");
#endif
					RedrawWindow(notifyWindow,NULL,NULL,RDW_INVALIDATE);
				}
			}
		}
	}
}

void AsynchTexLoader::loadTexImage(CollectionPos pos){
	int idx = pos.toIndex();
	if (textureCache.exists(idx)){ //already loaded
		return;
	}
#ifdef _DEBUG
		Console::println(L"                 x");
#endif
	ImgTexture * tex = collection->getImgTexture(pos);
	EnterCriticalSection(&textureCacheCS);
	textureCache.set(idx,tex);
	LeaveCriticalSection(&textureCacheCS);
	queueGlUpload(idx);
}

void AsynchTexLoader::cleanUpCache(){
	int textureCacheCount = textureCache.get_count();
	int deleteCount = textureCacheCount - textureCacheSize;
	if (deleteCount > 0){
		cleanUpCacheDistances.remove_all();
		cleanUpCacheDistances.prealloc(textureCacheCount);
		enumHelper.loadCenter = &loadCenter;
		enumHelper.collection = collection;
		enumHelper.cleanUpCacheDistances = &cleanUpCacheDistances;
		textureCache.enumerate(cleanUpCacheGetDistances);
		cleanUpCacheDistances.sort(cleanUpCacheDistances_compare());
		for (int i=0; i < deleteCount; i++){
			int idx = cleanUpCacheDistances.get_item_ref(i).cacheIdx;
			ImgTexture * tex;
			if (!textureCache.query(idx, tex))
				continue;
			EnterCriticalSection(&textureCacheCS);
			textureCache.remove(idx);
			LeaveCriticalSection(&textureCacheCS);
			queueGlDelete(tex);
		}
	}
}

void AsynchTexLoader::cleanUpCacheGetDistances(int idx, ImgTexture* tex){
	t_cleanUpCacheDistance dst;
	dst.cacheIdx = idx;
	dst.distance = abs(*(enumHelper.loadCenter) - CollectionPos(enumHelper.collection, idx));
	enumHelper.cleanUpCacheDistances->add_item(dst);
}

void AsynchTexLoader::queueGlUpload(int coverTexMapIdx){
	uploadQueue[uploadQueueIn] = coverTexMapIdx;
	int target = uploadQueueIn + 1; // needed because of multithreading
	if (target == UPLOAD_QUEUE_SIZE)
		target = 0;
	if (target != uploadQueueOut) // otherwise do nothing - this has no problematic consequences
		uploadQueueIn = target;
}

bool AsynchTexLoader::runGlUpload(unsigned int limit){
	unsigned int i=0;
	while (uploadQueueOut != uploadQueueIn){
		if (++i > limit){
			return false;
		}
		ImgTexture * tex;
		EnterCriticalSection(&textureCacheCS);
		bool exists = textureCache.query(uploadQueue[uploadQueueOut], tex);
		LeaveCriticalSection(&textureCacheCS);
		if (exists && tex)
			tex->glUpload();

		int target = uploadQueueOut + 1; // needed because of multithreading
		if (target == UPLOAD_QUEUE_SIZE)
			target = 0;
		uploadQueueOut = target;
	}
	return true;
}

void AsynchTexLoader::queueGlDelete(ImgTexture* tex){
	if (tex == 0)
		return;
	deleteBuffer[deleteBufferIn] = tex;
	int target = deleteBufferIn + 1; // needed because of multithreading
	if (target == DELETE_BUFFER_SIZE)
		target = 0;
	if (target == deleteBufferOut)
		WaitForSingleObject(deleteBufferFreed,INFINITE);
	deleteBufferIn = target;
}

void AsynchTexLoader::runGlDelete(){
	while (deleteBufferOut != deleteBufferIn){
		deleteBuffer[deleteBufferOut]->glDelete();
		delete deleteBuffer[deleteBufferOut];
		int target = deleteBufferOut + 1; // needed because of multithreading
		if (target == DELETE_BUFFER_SIZE)
			target = 0;
		deleteBufferOut = target;
	}
	SetEvent(deleteBufferFreed);
}