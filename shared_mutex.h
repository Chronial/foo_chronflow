#pragma once

#include "stdafx.h"

class shared_mutex
{
private:
	boost::shared_mutex mtx;
#ifdef _DEBUG
	std::mutex debug_mtx;
	std::unordered_multiset<DWORD> mtx_shared_owners;
	DWORD mtx_exclusive_owner;
#endif

public:
#ifdef _DEBUG
	shared_mutex(): mtx_exclusive_owner(0){};
#else
	shared_mutex(){};
#endif


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
};