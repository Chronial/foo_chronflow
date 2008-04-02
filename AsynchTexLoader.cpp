#include "chronflow.h"
#include <process.h>

extern cfg_string cfgImgNoCover;
extern cfg_string cfgImgLoading;

struct AsynchTexLoader::EnumHelper {
	CollectionPos* loadCenter;
	DbAlbumCollection* collection;
	pfc::list_t<AsynchTexLoader::t_cleanUpCacheDistance>* cleanUpCacheDistances;

	t_resynchCallback resynchCallback;
	void* resynchCallbackParam;
	t_textureCache* resynchCallbackOut;
	AlbumCollection* resynchCollection;
};
AsynchTexLoader::EnumHelper AsynchTexLoader::enumHelper = {0};

AsynchTexLoader::AsynchTexLoader(AppInstance* instance) :
queueCenter(instance->albumCollection,0),
loadCenter(instance->albumCollection, 0),
loadingTexture(0),
noCoverTexture(0),
workerThread(0),
deleteBufferIn(0),
deleteBufferOut(0),
uploadQueueHead(0),
uploadQueueTail(0),
glWindow(0),
glRC(0),
glDC(0),
appInstance(instance)
{
	InitializeCriticalSectionAndSpinCount(&textureCacheCS, 0x80000400);

	textureCacheSize = 50;
	//loadSpecialTextures();
	workerThreadHasWork = CreateEvent(NULL,FALSE,FALSE,NULL);

	ZeroMemory(deleteBuffer,sizeof(deleteBuffer));
	deleteBufferFreed = CreateEvent(NULL,FALSE,FALSE,NULL);

	mayUpload = CreateEvent(NULL, TRUE, TRUE, NULL);

	lockMainthreadRC = CreateEvent(NULL, FALSE, FALSE, NULL);
}

void AsynchTexLoader::loadSpecialTextures(){
	if (loadingTexture){
		loadingTexture->glDelete();
		delete loadingTexture;
	}
	pfc::string8 loadingTexPath(cfgImgLoading);
	Helpers::fixPath(loadingTexPath);
	loadingTexture = new ImgTexture(loadingTexPath);

	if (noCoverTexture){
		noCoverTexture->glDelete();
		delete noCoverTexture;
	}
	pfc::string8 noCoverTexPath(cfgImgNoCover);
	Helpers::fixPath(noCoverTexPath);
	noCoverTexture = new ImgTexture(noCoverTexPath);
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
	clearCache();
	if (uploadQueueHead){
		UploadQueueElem* prevE;
		UploadQueueElem* e = uploadQueueHead;
		while (e){
			prevE = e;
			e = prevE->next;
			delete prevE;
		}
	}
	loadingTexture->glDelete();
	delete loadingTexture;
	noCoverTexture->glDelete();
	delete noCoverTexture;
	runGlDelete();
	CloseHandle(workerThreadHasWork);
	CloseHandle(lockMainthreadRC);
	DeleteCriticalSection(&textureCacheCS);
}
void AsynchTexLoader::textureCacheDeleteEnumerator(int idx, ImgTexture* tex){
	if (tex){
		tex->glDelete();
		delete tex;
	}
}

ImgTexture* AsynchTexLoader::getLoadedImgTexture(CollectionPos pos)
{
	ImgTexture* tex;
	int idx = pos.toIndex();

	EnterCriticalSection(&textureCacheCS);
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
	appInstance->lockedRC.explicitRelease();

	closeWorkerThread = false;
	workerThread = (HANDLE)_beginthreadex(0,0,&(this->runWorkerThread),(void*)this,0,0);
	setWorkerThreadPrio(THREAD_PRIORITY_NORMAL, true);

	WaitForSingleObject(lockMainthreadRC, INFINITE);
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


bool AsynchTexLoader::initGlContext()
{
	double a[10];
	a[0] = Helpers::getHighresTimer();
	glWindow = CreateWindowEx( 
		WS_EX_NOACTIVATE,
		L"Chronflow",           // class name                   
		L"Loader Window",          // window name                  
		WS_DISABLED,
		CW_USEDEFAULT,          // default horizontal position  
		CW_USEDEFAULT,          // default vertical position    
		CW_USEDEFAULT,          // default width                
		CW_USEDEFAULT,          // default height               
		(HWND) NULL,            // no parent or owner window    
		(HMENU) NULL,           // class menu used              
		core_api::get_my_instance(),              // instance handle              
		NULL);                  // no window creation data      
	if (!glWindow){
		MessageBox(NULL,L"TexLoader: Couldn't create hidden Window.",L"Foo_chronflow Error",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}
	a[1] = Helpers::getHighresTimer();

	if (!(glDC=GetDC(glWindow)))							// Did We Get A Device Context?
	{
		destroyGlContext();								// Reset The Display
		MessageBox(NULL,L"TexLoader: Can't Create A GL Device Context.",L"Foo_chronflow Error",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	a[2] = Helpers::getHighresTimer();
	int pixelFormat = appInstance->renderer->getPixelFormat();
	PIXELFORMATDESCRIPTOR pfd;
	DescribePixelFormat(glDC, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

	a[3] = Helpers::getHighresTimer();

	if(!SetPixelFormat(glDC,pixelFormat,&pfd))		// Are We Able To Set The Pixel Format?
	{
		destroyGlContext();								// Reset The Display
		MessageBox(NULL,L"TexLoader: Can't Set The PixelFormat.",L"Foo_chronflow Error",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	a[4] = Helpers::getHighresTimer();
	if (!(glRC=wglCreateContext(glDC)))				// Are We Able To Get A Rendering Context?
	{
		destroyGlContext();								// Reset The Display
		MessageBox(NULL,L"TexLoader: Can't Create A GL Rendering Context.",L"Foo_chronflow Error",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	a[5] = Helpers::getHighresTimer();
	if (!appInstance->renderer->shareLists(glRC)){
		SetEvent(lockMainthreadRC);
		destroyGlContext();								// Reset The Display
		MessageBox(NULL,L"TexLoader: Couldn't share Display Lists.",L"Foo_chronflow Error",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}
	SetEvent(lockMainthreadRC);

	a[6] = Helpers::getHighresTimer();
	for (int i=0; i < 6; i++){
		console::printf("initGlContext: a%d - a%d: %dmsec", i, i+1, int((a[i+1] - a[i])*1000));
	}


	if(!wglMakeCurrent(glDC,glRC))					// Try To Activate The Rendering Context
	{
		destroyGlContext();								// Reset The Display
		MessageBox(NULL,L"TexLoader: Can't Activate The GL Rendering Context.",L"Foo_chronflow Error",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}


	return TRUE;
}

void AsynchTexLoader::destroyGlContext(){
	if (glRC)											// Do We Have A Rendering Context?
	{
		if (!wglDeleteContext(glRC))						// Are We Able To Delete The RC?
		{
			console::print(pfc::string8("Foo_chronflow Error: Release Of RC Failed. (mainWin) Details:") << format_win32_error(GetLastError()));
		}
		glRC=NULL;										// Set RC To NULL
	}

	if (glDC && !ReleaseDC(glWindow,glDC))					// Are We Able To Release The DC
	{
		MessageBox(NULL,L"Release Device Context Failed.",L"SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		glDC=NULL;										// Set DC To NULL
	}
	DestroyWindow(glWindow);
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
	enumHelper.resynchCollection = appInstance->albumCollection;
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
	bool onScreen = true;
	initGlContext();

	int notifyThreshold = 10;
	int notifyC = 0;
	//WaitForSingleObject(workerThreadHasWork,INFINITE);
	while(!closeWorkerThread){
		if (uploadQueueHead && WaitForSingleObject(mayUpload, 0) == WAIT_OBJECT_0){
			doUploadQueue();
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
			if (loadDistance == int((textureCacheSize - 1)*0.9)){
				cleanUpCache();
				cleanUpC = 0;
				if (uploadQueueHead){
					HANDLE handles[2] = {workerThreadHasWork, mayUpload};
					WaitForMultipleObjects(2, handles, FALSE, INFINITE);
				} else {
					WaitForSingleObject(workerThreadHasWork,INFINITE);
				}
			} else {
				loadDistance++;
				int loadIdx;
				if (loadDistance % 2){
					loadIdx = (loadDistance + 1) / 2;
				} else {
					loadIdx = -loadDistance / 2;
				}
				// unclean / hack - this is not mulithread safe!
				onScreen = (loadIdx >= appInstance->coverPos->getFirstCover()) && (loadIdx <= appInstance->coverPos->getLastCover());

				if (loadDistance < 8){
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
				if ((!nearCenter && cleanUpC > 10 && cleanUpC > textureCacheSize * 0.3)
					|| (cleanUpC > 30)){
					cleanUpCache();
					cleanUpC = 0;
				}
				CollectionPos loadTarget = loadCenter + loadIdx;
				IF_DEBUG(Console::printf(L"L d:%2d - idx:%4d\n",loadDistance, loadTarget.toIndex()));
				
				bool doUpload = false;
				if (/*(loadDistance < (textureCacheSize / 2))
					|| */(WaitForSingleObject(mayUpload, 0) == WAIT_OBJECT_0)){
					doUpload = true;
				}
				loadTexImage(loadTarget, doUpload);
				{ // Redraw Mainwindow
					if ((nearCenter && (notifyC % 2)) ||
						(onScreen && ((notifyC % 4) == 0)) ||
						(loadDistance == 0) ||
						(++notifyC == notifyThreshold)){
						notifyC = 0;
						notifyThreshold = loadDistance * loadDistance / textureCacheSize;
						IF_DEBUG(Console::println(L"                    Refresh trig."));
						RedrawWindow(appInstance->mainWindow,NULL,NULL,RDW_INVALIDATE);
					}
				}
			}
		}
	}
	destroyGlContext();
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
	EnterCriticalSection(&textureCacheCS);
	textureCache.set(idx,tex);
	LeaveCriticalSection(&textureCacheCS);
}

void AsynchTexLoader::allowUpload(){
	SetEvent(mayUpload);
}

void AsynchTexLoader::blockUpload(){
	ResetEvent(mayUpload);
}

void AsynchTexLoader::doUploadQueue(){
	UploadQueueElem* e = uploadQueueHead;
	ImgTexture* tex;
	while (e){
		if (WaitForSingleObject(mayUpload, 0) != WAIT_OBJECT_0)
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
	int deleteCount = textureCacheCount - textureCacheSize;
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