#include "chronflow.h"
#include <process.h>

AsynchTexLoader::AsynchTexLoader(AlbumCollection* collection)
: queueCenter(collection,0), loadCenter(collection, 0)
{
	this->notifyWindow = 0;
	this->collection = collection;
	coverTexMap = new int[collection->getCount()];
	memset(coverTexMap,-1,sizeof(int)*collection->getCount());
	InitializeCriticalSection(&coverTexMapCS);

	textureBufferSize = 100; // this has to be larger than the amount of currently displayed covers!
	if (textureBufferSize > collection->getCount())
		textureBufferSize = collection->getCount();
	textures = new ImgTexture*[textureBufferSize];
	ZeroMemory(textures,sizeof(textures[0])*textureBufferSize);
	texturePos = new CollectionPos*[textureBufferSize];
	ZeroMemory(texturePos,sizeof(texturePos[0])*textureBufferSize);

	defaultTexture = new ImgFileTexture(L"default.png");
	workerThreadHasWork = CreateEvent(NULL,FALSE,FALSE,NULL);

	ZeroMemory(deleteBuffer,sizeof(deleteBuffer));
	deleteBufferIn = deleteBufferOut = 0;
	deleteBufferFreed = CreateEvent(NULL,FALSE,FALSE,NULL);

	uploadQueueIn = uploadQueueOut = 0;
	
	startWorkerThread();
}

AsynchTexLoader::~AsynchTexLoader(void)
{
	stopWorkerThread();
	for (int i=0; i < textureBufferSize; ++i){
		if (textures[i]){
			textures[i]->glDelete();
			delete textures[i];
			delete texturePos[i];
		}
	}
	delete textures;
	delete texturePos;
	delete coverTexMap;
	defaultTexture->glDelete();
	delete defaultTexture;
	CloseHandle(workerThreadHasWork);
	DeleteCriticalSection(&coverTexMapCS);
}

void AsynchTexLoader::setNotifyWindow(HWND hWnd){
	this->notifyWindow = hWnd;
}

ImgTexture* AsynchTexLoader::getLoadedImgTexture(CollectionPos pos)
{
	ImgTexture* tex;
	int idx = pos.toIndex();
	EnterCriticalSection(&coverTexMapCS);
	if (coverTexMap[idx] > -1)
		tex = textures[coverTexMap[idx]];
	else
		tex = defaultTexture;
	LeaveCriticalSection(&coverTexMapCS);
	return tex;
}

void AsynchTexLoader::setQueueCenter(CollectionPos center)
{
	if (!(center == queueCenter)){
		queueCenter = center;
		SetEvent(workerThreadHasWork);
	}
}


void AsynchTexLoader::startWorkerThread()
{
	closeWorkerThread = false;
	workerThread = (HANDLE)_beginthreadex(0,0,&(this->runWorkerThread),(void*)this,0,0);
	SetThreadPriority(workerThread,THREAD_PRIORITY_IDLE);
}

unsigned int WINAPI AsynchTexLoader::runWorkerThread(void* lpParameter)
{
	((AsynchTexLoader*)(lpParameter))->workerThreadProc();
	return 0;
}

void AsynchTexLoader::stopWorkerThread()
{
	closeWorkerThread = true;
	SetEvent(workerThreadHasWork);
	if (workerThread){
		WaitForSingleObject(workerThread,INFINITE);
		CloseHandle(workerThread);
		workerThread = 0;
	}
}

void AsynchTexLoader::workerThreadProc()
{
	loadDistance = -1;
	loadCenter = queueCenter;
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
		if (loadDistance == int((textureBufferSize - 1)*0.9)){
			WaitForSingleObject(workerThreadHasWork,INFINITE);
		} else {
			loadDistance++;

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
					((loadDistance < 15) && (notifyC % 2))){
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
	if (coverTexMap[idx] > -1){ //already loaded
		return;
	}
#ifdef _DEBUG
		Console::println(L"                 x");
#endif
	int maxDist = 0;
	int tPos = 0;
	for (int i=0; i < textureBufferSize; i++){ // this grows quadratic with buffer sice - not good
		if (!textures[i]){
			tPos = i;
			maxDist = -1;
			break;
		}
		int dist = abs(loadCenter - *(texturePos[i]));
		if (dist > maxDist){
			tPos = i;
			maxDist = dist;
		}
	}
	if (maxDist == 0){
		MessageBox(NULL,L"There was no space in the textureBuffer, killing texture loading thread",L"Chronflow Error",MB_OK|MB_ICONERROR);
		closeWorkerThread = true;
		return;
	}
	if (maxDist > 0){ //delete old Texture
		ImgTexture* oldTex = textures[tPos];
		coverTexMap[texturePos[tPos]->toIndex()] = -1;
		delete texturePos[tPos];
		texturePos[tPos] = 0;
		textures[tPos] = 0;
		queueGlDelete(oldTex);
	}
	textures[tPos]= collection->getImgTexture(pos);
	texturePos[tPos] = new CollectionPos(pos);
	EnterCriticalSection(&coverTexMapCS);
	coverTexMap[idx] = tPos;
	LeaveCriticalSection(&coverTexMapCS);
	queueGlUpload(idx);
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
		EnterCriticalSection(&coverTexMapCS);
		int texIdx = coverTexMap[uploadQueue[uploadQueueOut]];
		if (texIdx > -1){
			textures[texIdx]->glUpload();
		}
		LeaveCriticalSection(&coverTexMapCS);
		int target = uploadQueueOut + 1; // needed because of multithreading
		if (target == UPLOAD_QUEUE_SIZE)
			target = 0;
		uploadQueueOut = target;
	}
	return true;
}

void AsynchTexLoader::queueGlDelete(ImgTexture* tex){
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