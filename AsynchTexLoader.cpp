#include "stdafx.h"
#include <process.h>
#include "config.h"
#include "Helpers.h"

#include "AsynchTexLoader.h"

#include "AppInstance.h"
#include "Console.h"
#include "ScriptedCoverPositions.h"
#include "DbAlbumCollection.h"
#include "ImgTexture.h"
#include "RenderThread.h"


struct AsynchTexLoader::EnumHelper {
	CollectionPos* loadCenter;
	DbAlbumCollection* collection;
	pfc::list_t<AsynchTexLoader::t_cleanUpCacheDistance>* cleanUpCacheDistances;

	t_resynchCallback resynchCallback;
	void* resynchCallbackParam;
	t_textureCache* resynchCallbackOut;
	DbAlbumCollection* resynchCollection;
};
AsynchTexLoader::EnumHelper AsynchTexLoader::enumHelper = {0};

AsynchTexLoader::AsynchTexLoader(AppInstance* instance) :
appInstance(instance),
queueCenter(instance->albumCollection,0),
loadCenter(instance->albumCollection, 0),
loadingTexture(0),
noCoverTexture(0),
workerThread(0),
deleteBufferIn(0),
deleteBufferOut(0),
uploadQueueHead(0),
uploadQueueTail(0),
glRC(0),
glDC(0),
workerThreadMayRun(true, true),
mayUpload(true, true)
{
	loadSpecialTextures();
	createLoaderWindow();
	ZeroMemory(deleteBuffer,sizeof(deleteBuffer));
	startWorkerThread();
}

void AsynchTexLoader::loadSpecialTextures(){
	if (loadingTexture){
		loadingTexture->glDelete();
		delete loadingTexture;
	}
	pfc::string8 loadingTexPath(cfgImgLoading);
	loadingTexPath.skip_trailing_char(' ');
	if (loadingTexPath.get_length() > 0){
		Helpers::fixPath(loadingTexPath);
		loadingTexture = new ImgTexture(loadingTexPath);
	}
	else {
		loadingTexture = new ImgTexture(IDR_COVER_LOADING, L"JPG");
	}

	if (noCoverTexture){
		noCoverTexture->glDelete();
		delete noCoverTexture;
	}
	pfc::string8 noCoverTexPath(cfgImgNoCover);
	noCoverTexPath.skip_trailing_char(' ');
	if (noCoverTexPath.get_length() > 0){
		Helpers::fixPath(noCoverTexPath);
		noCoverTexture = new ImgTexture(noCoverTexPath);
	}
	else {
		noCoverTexture = new ImgTexture(IDR_COVER_NO_IMG, L"JPG");
	}

}

AsynchTexLoader::~AsynchTexLoader(void)
{
	IF_DEBUG(Console::println(L"Destroying AsynchTexLoader"));
	stopWorkerThread();
	if (uploadQueueHead){
		UploadQueueElem* prevE;
		UploadQueueElem* e = uploadQueueHead;
		while (e){
			prevE = e;
			e = prevE->next;
			delete prevE;
		}
	}
	// ugly, do better? - maybe window hook
	if (IsWindow(glWindow)){
		IF_DEBUG(Console::println(L" - Window still existed"));
		clearCache();
		reserveRenderContext();
		loadingTexture->glDelete();
		noCoverTexture->glDelete();
		runGlDelete();
		releaseRenderContext();
	} else {
		IF_DEBUG(Console::println(L" - Window was already gone"));
		// Has no effect, but prevents error message
		loadingTexture->glDelete();
		noCoverTexture->glDelete();
		// Do this even if we can’t delete
		textureCache.enumerate(textureCacheDeleteEnumerator);
	}
	destroyLoaderWindow();
	delete loadingTexture;
	delete noCoverTexture;
}

void AsynchTexLoader::textureCacheDeleteEnumerator(int idx, ImgTexture* tex){
	if (tex){
		tex->glDelete();
		delete tex;
	}
}

void AsynchTexLoader::createLoaderWindow(){
	glWindow = CreateWindowEx( 
		0,
		L"Chronflow LoaderWindow",        // class name                   
		L"Chronflow Loader Window",       // window name                  
		WS_DISABLED | WS_CHILD,
		CW_USEDEFAULT,          // default horizontal position  
		CW_USEDEFAULT,          // default vertical position    
		CW_USEDEFAULT,          // default width                
		CW_USEDEFAULT,          // default height               
		appInstance->mainWindow,// parent or owner window    
		(HMENU) NULL,           // class menu used              
		core_api::get_my_instance(),// instance handle              
		NULL);                  // no window creation data

	if (!glWindow){
		errorPopupWin32("TexLoader failed to create a window.");
		throw new pfc::exception();
	}

	if (!(glDC = GetDC(glWindow))){
		errorPopupWin32("TexLoader failed to get a Device Context");
		destroyLoaderWindow();
		throw new pfc::exception();
	}

	int pixelFormat = appInstance->renderer->getPixelFormat();
	PIXELFORMATDESCRIPTOR pfd;
	DescribePixelFormat(glDC, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

	if(!SetPixelFormat(glDC,pixelFormat,&pfd)){
		errorPopupWin32("TexLoader failed to set PixelFormat");
		destroyLoaderWindow();
		throw new pfc::exception();
	}

	if (!(glRC = wglCreateContext(glDC))){
		errorPopupWin32("TexLoader failed to create a Rendering Context");
		destroyLoaderWindow();
		throw new pfc::exception();
	}

	if (!appInstance->renderer->shareLists(glRC)){
		errorPopupWin32("TexLoader failed to share Display Lists");
		destroyLoaderWindow();
		throw new pfc::exception();
	}

	wglMakeCurrent(NULL, NULL);
}



ImgTexture* AsynchTexLoader::getLoadedImgTexture(CollectionPos pos)
{
	ImgTexture* tex;
	int idx = pos.toIndex();

	textureCacheCS.enter();
	if(textureCache.query(idx, tex)){
		if (tex == 0)
			tex = noCoverTexture;
	} else {
		tex = loadingTexture;
	}
	textureCacheCS.leave();
	return tex;
}

void AsynchTexLoader::setQueueCenter(CollectionPos center)
{
	if (!(center == queueCenter)){
		queueCenter = center;
		workerThreadHasWork.setSignal();
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
	workerThreadMayRun.resetSignal();
	workerThread = (HANDLE)_beginthreadex(0,0,&(this->runWorkerThread),(void*)this,0,0);
	setWorkerThreadPrio(cfgTexLoaderPrio, true);
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
		workerThreadMayRun.setSignal();
		workerThreadHasWork.setSignal();
		WaitForSingleObject(workerThread,INFINITE);
		IF_DEBUG(Console::println(L"AsyncTexLoader worker thread stopped"));
		CloseHandle(workerThread);
		workerThread = 0;
	}
}

void AsynchTexLoader::pauseLoading()
{
	workerThreadMayRun.resetSignal();
}

void AsynchTexLoader::resumeLoading()
{
	workerThreadHasWork.setSignal();
	workerThreadMayRun.setSignal();
}

void AsynchTexLoader::destroyLoaderWindow()
{
	if (glRC){
		if (!wglDeleteContext(glRC)){
			errorPopupWin32("failed to destroy  AsyncTexLoader Render Context");
		}
		glRC = NULL;
	}

	if (glWindow)
		DestroyWindow(glWindow);
}

void AsynchTexLoader::reserveRenderContext()
{
	renderContext.enter();
	if (!wglMakeCurrent(glDC, glRC)){
		IF_DEBUG(errorPopupWin32("failed to reserve AsyncTexLoader Render Context"));
	}
}

void AsynchTexLoader::releaseRenderContext()
{
	if (!wglMakeCurrent(NULL, NULL)){
		IF_DEBUG(errorPopupWin32("failed to release AsyncTexLoader Render Context"));
	}
	renderContext.leave();
}


void AsynchTexLoader::clearCache()
{
	ScopeCS scopeLock(workerThreadInLoop);
	reserveRenderContext();
	textureCache.enumerate(textureCacheDeleteEnumerator);
	textureCache.remove_all();
	loadDistance = -1;
	releaseRenderContext();
}

void AsynchTexLoader::resynchCache(t_resynchCallback callback, void* param)
{
	ScopeCS scopeLock(workerThreadInLoop);
	reserveRenderContext();
	t_textureCache oldCache = textureCache;
	textureCache.remove_all();
	enumHelper.resynchCallback = callback;
	enumHelper.resynchCallbackParam = param;
	enumHelper.resynchCallbackOut = &textureCache;
	enumHelper.resynchCollection = appInstance->albumCollection;
	oldCache.enumerate(resynchCacheEnumerator);
	loadDistance = -1;
	releaseRenderContext();
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
	bool onScreen = true;
	int notifyThreshold = 10;
	int notifyC = 0;
	while(!closeWorkerThread){
		workerThreadMayRun.waitForSignal();

		if (closeWorkerThread)
			break;

		if (uploadQueueHead && mayUpload.waitForSignal(0)){
			reserveRenderContext();
			doUploadQueue();
			releaseRenderContext();
		} else {
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

			if ((loadDistance >= int((cfgTextureCacheSize - 1)*0.9)) ||
				((loadDistance+1) >= appInstance->albumCollection->getCount())){
				cleanUpCache();
				cleanUpC = 0;
				if (uploadQueueHead){
					workerThreadHasWork.waitForTwo(mayUpload, false);
				} else {
					workerThreadHasWork.waitForSignal();
				}
			} else {
				ScopeCS scopeLock(workerThreadInLoop);
				loadDistance++;
				int loadIdx;
				if (loadDistance % 2){
					loadIdx = (loadDistance + 1) / 2;
				} else {
					loadIdx = -loadDistance / 2;
				}
				// unclean / hack - this is not mulithread safe!
				onScreen = (loadIdx >= appInstance->coverPos->getFirstCover()) && (loadIdx <= appInstance->coverPos->getLastCover());

				if (loadDistance < 16){
					if (!nearCenter){
						nearCenter = true;
						setWorkerThreadPrio(cfgTexLoaderPrio);
					}
				} else {
					if (nearCenter){
						nearCenter = false;
						setWorkerThreadPrio(THREAD_PRIORITY_IDLE);
					}
				}
				if ((cleanUpC % (int(ceil(cfgTextureCacheSize * 0.2))+1)) == 0){
					cleanUpCache();
					cleanUpC = 0;
				}
				CollectionPos loadTarget = loadCenter + loadIdx;
				IF_DEBUG(Console::printf(L"L d:%2d - idx:%4d\n",loadDistance, loadTarget.toIndex()));
				
				bool doUpload = false;
				if (mayUpload.waitForSignal(0)){
					doUpload = true;
				}
				reserveRenderContext();
				loadTexImage(loadTarget, doUpload);
				releaseRenderContext();
				{ // Redraw Mainwindow
					if ((nearCenter && (notifyC % 2)) ||
						(onScreen && ((notifyC % 4) == 0)) ||
						(loadDistance == 0) ||
						(++notifyC == notifyThreshold)){
						notifyC = 0;
						notifyThreshold = loadDistance * loadDistance / cfgTextureCacheSize;
						IF_DEBUG(Console::println(L"                    Refresh trig."));
						appInstance->redrawMainWin();
					}
				}
			}
		}
	}
}

void AsynchTexLoader::loadTexImage(CollectionPos pos, bool doUpload){
	int idx = pos.toIndex();
	ImgTexture * tex;
	if (textureCache.query(idx, tex)){ //already loaded
		if (doUpload && tex)
			tex->glUpload();
		return;
	}
	IF_DEBUG(Console::println(L"                 x"));
	tex = appInstance->albumCollection->getImgTexture(pos);
	if (doUpload && tex){
		tex->glUpload();
	} else {
		UploadQueueElem* e = new UploadQueueElem;
		e->next = 0;
		e->texIdx = idx;
		if (uploadQueueTail)
			uploadQueueTail->next = e;
		else
			uploadQueueHead = e;
		uploadQueueTail = e;
	}
	textureCacheCS.enter();
	textureCache.set(idx,tex);
	textureCacheCS.leave();
}

void AsynchTexLoader::allowUpload(){
	mayUpload.setSignal();
}

void AsynchTexLoader::blockUpload(){
	mayUpload.resetSignal();
}

void AsynchTexLoader::doUploadQueue(){
	UploadQueueElem* e = uploadQueueHead;
	ImgTexture* tex;
	while (e){
		if (!mayUpload.waitForSignal(0))
			break;
		if(textureCache.query(e->texIdx, tex)){
			if (tex != 0){
				tex->glUpload();
			}
		}
		if (e == uploadQueueTail)
			uploadQueueTail = 0;
		uploadQueueHead = e->next;
		delete e;
		e = uploadQueueHead;
	}
}

void AsynchTexLoader::cleanUpCache(){
	int textureCacheCount = textureCache.get_count();
	int deleteCount = textureCacheCount - cfgTextureCacheSize;
	if (deleteCount > 0){
		cleanUpCacheDistances.remove_all();
		cleanUpCacheDistances.prealloc(textureCacheCount);
		enumHelper.loadCenter = &loadCenter;
		enumHelper.collection = appInstance->albumCollection;
		enumHelper.cleanUpCacheDistances = &cleanUpCacheDistances;
		textureCache.enumerate(cleanUpCacheGetDistances);
		cleanUpCacheDistances.sort(cleanUpCacheDistances_compare());
		for (int i=0; i < deleteCount; i++){
			int idx = cleanUpCacheDistances.get_item_ref(i).cacheIdx;
			ImgTexture * tex;
			if (!textureCache.query(idx, tex))
				continue;
			textureCacheCS.enter();
			textureCache.remove(idx);
			textureCacheCS.leave();
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

void AsynchTexLoader::queueGlDelete(ImgTexture* tex){
	if (tex == 0)
		return;
	deleteBuffer[deleteBufferIn] = tex;
	int target = deleteBufferIn + 1; // needed because of multithreading
	if (target == DELETE_BUFFER_SIZE)
		target = 0;
	if (target == deleteBufferOut)
		deleteBufferFreed.waitForSignal();
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
	deleteBufferFreed.setSignal();
}