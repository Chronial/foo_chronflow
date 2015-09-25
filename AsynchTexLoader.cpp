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


AsynchTexLoader::AsynchTexLoader(AppInstance* instance) :
appInstance(instance)
{
	createLoaderWindow();
}

void AsynchTexLoader::start(){
	workerThread = (HANDLE)_beginthreadex(0, 0, [](void* self) -> unsigned int {
		static_cast<AsynchTexLoader*>(self)->threadProc();
		return 0;
	}, (void*)this, 0, 0);
	setWorkerThreadPrio(cfgTexLoaderPrio, true);
}

void AsynchTexLoader::loadSpecialTextures(){
	// lock textureCache to synchronize with texture getter
	auto lock = textureCache.synchronize();
	if (loadingTexture){
		loadingTexture->glDelete();
	}
	pfc::string8 loadingTexPath(cfgImgLoading);
	loadingTexPath.skip_trailing_char(' ');
	if (loadingTexPath.get_length() > 0){
		Helpers::fixPath(loadingTexPath);
		loadingTexture = make_shared<ImgTexture>(loadingTexPath);
	}
	else {
		loadingTexture = make_shared<ImgTexture>(IDR_COVER_LOADING, L"JPG");
	}

	if (noCoverTexture){
		noCoverTexture->glDelete();
	}
	pfc::string8 noCoverTexPath(cfgImgNoCover);
	noCoverTexPath.skip_trailing_char(' ');
	if (noCoverTexPath.get_length() > 0){
		Helpers::fixPath(noCoverTexPath);
		noCoverTexture = make_shared<ImgTexture>(noCoverTexPath);
	}
	else {
		noCoverTexture = make_shared<ImgTexture>(IDR_COVER_NO_IMG, L"JPG");
	}
}

AsynchTexLoader::~AsynchTexLoader(void)
{
	IF_DEBUG(Console::println(L"Destroying AsynchTexLoader"));
	if (workerThread){
		this->send(make_shared<ATStopThreadMessage>());
		WaitForSingleObject(workerThread, INFINITE);
		IF_DEBUG(Console::println(L"AsyncTexLoader worker thread stopped"));
		CloseHandle(workerThread);
		workerThread = nullptr;
	}
	ASSERT(IsWindow(glWindow));
	DestroyWindow(glWindow);
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
		(HMENU)NULL,           // class menu used              
		core_api::get_my_instance(),// instance handle              
		NULL);                  // no window creation data

	if (!glWindow){
		errorPopupWin32("TexLoader failed to create a window.");
		throw new pfc::exception();
	}
}

void AsynchTexLoader::initGlContext(){
	if (!(glDC = GetDC(glWindow))){
		errorPopupWin32("TexLoader failed to get a Device Context");
		destroyGlContext();
		throw new pfc::exception();
	}

	int pixelFormat = appInstance->renderer->getPixelFormat();
	PIXELFORMATDESCRIPTOR pfd;
	DescribePixelFormat(glDC, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

	if(!SetPixelFormat(glDC,pixelFormat,&pfd)){
		errorPopupWin32("TexLoader failed to set PixelFormat");
		destroyGlContext();
		throw new pfc::exception();
	}

	if (!(glRC = wglCreateContext(glDC))){
		errorPopupWin32("TexLoader failed to create a Rendering Context");
		destroyGlContext();
		throw new pfc::exception();
	}

	if (!appInstance->renderer->shareLists(glRC)){
		errorPopupWin32("TexLoader failed to share Display Lists");
		destroyGlContext();
		throw new pfc::exception();
	}

	wglMakeCurrent(glDC, glRC);
}

void AsynchTexLoader::destroyGlContext()
{
	if (glRC){
		if (!wglDeleteContext(glRC)){
			errorPopupWin32("failed to destroy AsyncTexLoader Render Context");
		}
		glRC = nullptr;
	}

	if (glDC){
		if (!ReleaseDC(appInstance->mainWindow, glDC)){
			errorPopupWin32("failed to release AsyncTexLoader Device Context");
		}
		glDC = nullptr;
	}
}


shared_ptr<ImgTexture> AsynchTexLoader::getLoadedImgTexture(CollectionPos pos)
{
	auto lockedCache = textureCache.synchronize();
	shared_ptr<ImgTexture> tex;
	auto entry = lockedCache->find(pos->groupString);
	if (entry != lockedCache->end()){
		if (entry->texture)
			return entry->texture;
		else
			return noCoverTexture;
	} else {
		return loadingTexture;
	}
}

void AsynchTexLoader::setWorkerThreadPrio(int nPrio, bool force)
{
	if (workerThreadPrio != nPrio || force){
		workerThreadPrio = nPrio;
		SetThreadPriority(workerThread,nPrio);
	}
}


void AsynchTexLoader::send(shared_ptr<ATMessage> msg){
	messageQueue.push(msg);
}

void AsynchTexLoader::threadProc()
{
	// TODO: make sure this thread is stopped before its window is destroyed
	//       maybe just pass a message processor. But renderer does probably not know then - is that a problem?
	initGlContext();
	loadSpecialTextures();

	for (;;){
		if (messageQueue.size() == 0){
			tryGlStuff();
			if (!isPaused && !allLoaded){
				allLoaded = !loadNextTexture();
				continue;
			}
		}
		auto msg = messageQueue.pop();

		if (auto m = dynamic_pointer_cast<ATStopThreadMessage>(msg)){
			break;
		} else if (auto m = dynamic_pointer_cast<ATRenderingDoneMessage>(msg)){
			// do nothing here, we just want this thread to wake up
		} else if (auto m = dynamic_pointer_cast<ATReloadSpecialTexMessage>(msg)){
			loadSpecialTextures();
		} else if (auto m = dynamic_pointer_cast<ATCollectionReloadMessage>(msg)){
			m->haltThread();
			clearCache();
			{
				collection_read_lock lock(appInstance);
				queueCenter = appInstance->albumCollection->getTargetPos();
			}
			allLoaded = false;
		} else if (auto m = dynamic_pointer_cast<ATClearCacheMessage>(msg)){
			clearCache();
		} else if (auto m = dynamic_pointer_cast<ATPauseMessage>(msg)){
			isPaused = true;
		} else if (auto m = dynamic_pointer_cast<ATResumeMessage>(msg)){
			isPaused = false;
		} else if (auto m = dynamic_pointer_cast<ATTargetChangedMessage>(msg)){
			collection_read_lock lock(appInstance);
			cacheGeneration++;
			// wrap around
			if (cacheGeneration > std::numeric_limits<unsigned int>::max() - 100){
				cacheGeneration = 1;
				auto lockedCache = textureCache.synchronize();
				for (auto it = lockedCache->begin(); it != lockedCache->end(); ++it){
					lockedCache->modify(it, [&](CacheItem& x) {
						x.priority.first = 0;
					});
				}
			}
			queueCenter = appInstance->albumCollection->getTargetPos();
			allLoaded = false;
		} else {
			IF_DEBUG(__debugbreak());
		}
	}
	loadingTexture->glDelete();
	noCoverTexture->glDelete();
	clearCache();
	destroyGlContext();
}

void AsynchTexLoader::clearCache(){
	std::unique_lock<std::mutex> uploadLock(openglAccess);
	auto lockedCache = textureCache.synchronize();

	for (auto &e : *lockedCache){
		if (e.texture){
			e.texture->glDelete();
		}
	}
	lockedCache->clear();
}

void AsynchTexLoader::tryGlStuff(){
	auto lockedCache = textureCache.synchronize();
	while (lockedCache->size() > (size_t)cfgTextureCacheSize){
		std::unique_lock<std::mutex> uploadLock(openglAccess, std::try_to_lock);
		if (!uploadLock){
			return;
		} else {
			auto &prorityIndex = lockedCache->get<1>();
			auto deleteTexture = prorityIndex.begin()->texture;
			prorityIndex.erase(prorityIndex.begin());
			if (deleteTexture)
				deleteTexture->glDelete();
		}
	}
	auto &uploadedIndex = lockedCache->get<2>();
	while (int c = uploadedIndex.count(false)){
		std::unique_lock<std::mutex> uploadLock(openglAccess, std::try_to_lock);
		if (!uploadLock){
			return;
		} else {
			auto toUpload = uploadedIndex.find(false);
			if (toUpload->texture)
				toUpload->texture->glUpload();
			uploadedIndex.modify_key(toUpload, [](bool &isUploaded) {isUploaded = true; });
		}
	}
}

bool AsynchTexLoader::loadNextTexture(){
	collection_read_lock lock(appInstance);

	int collectionSize = appInstance->albumCollection->getCount();
	if (collectionSize == 0){
		return false;
	}
	unsigned int maxLoad = min(collectionSize, int(cfgTextureCacheSize*0.8));
	unsigned int loaded = 0;

	CollectionPos leftLoaded = queueCenter;
	CollectionPos rightLoaded = queueCenter;
	CollectionPos loadNext = queueCenter;

	{
		auto lockedCache = textureCache.synchronize();
		for (; lockedCache->count(loadNext->groupString); ++loaded){
			// make sure the cache contains the right distances
			auto cacheEntry = lockedCache->find(loadNext->groupString);
			if (cacheEntry->priority.first != cacheGeneration){
				lockedCache->modify(cacheEntry, [=](CacheItem &x) {
					x.priority = std::make_pair(cacheGeneration, -(int)loaded);
				});
			}

			if (loaded >= maxLoad)
				return false;
			if ((loaded % 2 || leftLoaded == appInstance->albumCollection->begin()) &&
				++CollectionPos(rightLoaded) != appInstance->albumCollection->end()){
				++rightLoaded;
				loadNext = rightLoaded;
				ASSERT(loadNext != appInstance->albumCollection->end());
			} else {
				--leftLoaded;
				loadNext = leftLoaded;
			}
		}
	}

	auto texture = loadTexImage(loadNext);

	// update TextureCache
	textureCache->insert({ loadNext->groupString, { cacheGeneration, -(int)loaded }, texture, false });

	bool nearCenter = loaded < 16;

	if ((nearCenter && (loaded % 2)) ||
		!(loaded % 8)){
		IF_DEBUG(Console::println(L"Refresh MainWin."));
		appInstance->redrawMainWin();
	}
	return true;


	// unclean / hack - this is not mulithread safe!
	//onScreen = (loadOffset >= appInstance->coverPos->getFirstCover()) && (loadOffset <= appInstance->coverPos->getLastCover());

	/*
	if (loaded < 16){
		if (!nearCenter){
			nearCenter = true;
			setWorkerThreadPrio(cfgTexLoaderPrio);
		}
	} else {
		if (nearCenter){
			nearCenter = false;
			setWorkerThreadPrio(THREAD_PRIORITY_IDLE);
		}
	}*/

}

shared_ptr<ImgTexture> AsynchTexLoader::loadTexImage(CollectionPos pos){
	IF_DEBUG(double preLoad = Helpers::getHighresTimer());
	shared_ptr<ImgTexture> tex = appInstance->albumCollection->getImgTexture(pos);
#ifdef _DEBUG
	if (tex){
		Console::printf(L"Load image file in %d ms\n", int((Helpers::getHighresTimer() - preLoad) * 1000));
	} else {
		Console::printf(L"Detect missing file in %d ms\n", int((Helpers::getHighresTimer() - preLoad) * 1000));
	}
#endif
	return tex;
}

void AsynchTexLoader::trimCache(){
	auto lockedCache = textureCache.synchronize();
	while (lockedCache->size() > (size_t)cfgTextureCacheSize){
		auto &prorityIndex = lockedCache->get<1>();

		auto deleteTexture = prorityIndex.begin()->texture;
		prorityIndex.erase(prorityIndex.begin());
		if (deleteTexture)
			deleteTexture->glDelete();
	}
}