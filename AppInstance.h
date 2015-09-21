#pragma once

class AsynchTexLoader;
class DbAlbumCollection;
class RenderThread;
class DisplayPosition;
class PlaybackTracer;
class ScriptedCoverPositions;

class AppInstance
{
public:
	AppInstance () {
		texLoader = 0;
		albumCollection = 0;
		renderer = 0;
		displayPos = 0;
		playbackTracer = 0;
		mainWindow = 0;
		IF_DEBUG(mtx_exclusive_owner = 0);
	}

	AsynchTexLoader* texLoader;
	DbAlbumCollection* albumCollection;
	RenderThread* renderer;
	DisplayPosition* displayPos;
	PlaybackTracer* playbackTracer;
	HWND mainWindow;

	inline void redrawMainWin(){
		RedrawWindow(mainWindow,NULL,NULL,RDW_INVALIDATE);
	}
	inline bool isMainWinMinimized(){
		return !static_api_ptr_t<ui_control>()->is_visible();
	}

	void lock_shared() {
		mtx.lock_shared();
#ifdef _DEBUG
		{
			std::unique_lock<std::mutex> guard(debug_mtx);
			mtx_shared_owners.insert(GetCurrentThreadId());
		}
#endif
	}
	void unlock_shared() {
#ifdef _DEBUG
		{
			std::unique_lock<std::mutex> guard(debug_mtx);
			DWORD threadId = GetCurrentThreadId();
			auto owner = mtx_shared_owners.find(threadId);
			if (owner == mtx_shared_owners.end())
				__debugbreak();
			else
				mtx_shared_owners.erase(owner);
		}
#endif
		mtx.unlock_shared();
	}
	void lock() {
		mtx.lock();
#ifdef _DEBUG
		{
			std::unique_lock<std::mutex> guard(debug_mtx);
			assert(mtx_exclusive_owner == 0);
			mtx_exclusive_owner = GetCurrentThreadId();
		}
#endif
	}
	void unlock() {
#ifdef _DEBUG
		{
			std::unique_lock<std::mutex> guard(debug_mtx);
			assert(mtx_exclusive_owner == GetCurrentThreadId());
			mtx_exclusive_owner = 0;
		}
#endif
		mtx.unlock();
	}

#ifdef _DEBUG
	void assert_shared() {
		std::unique_lock<std::mutex> guard(debug_mtx);
		DWORD threadId = GetCurrentThreadId();
		if (mtx_shared_owners.count(threadId) < 1 && mtx_exclusive_owner != threadId){
			__debugbreak();
		}
	}
	void assert_exclusive() {
		std::unique_lock<std::mutex> guard(debug_mtx);
		if (mtx_exclusive_owner != GetCurrentThreadId())
			__debugbreak();
	}
#endif

private:
	shared_mutex mtx;
#ifdef _DEBUG
	std::mutex debug_mtx;
	std::unordered_multiset<DWORD> mtx_shared_owners;
	DWORD mtx_exclusive_owner;
#endif

};


#ifdef _DEBUG
#define ASSERT_APP_SHARED(X) X->assert_shared()
#define ASSERT_APP_EXCLUSIVE(X) X->assert_exclusive()
#else
#define ASSERT_APP_SHARED(X)
#define ASSERT_APP_EXCLUSIVE(X)
#endif