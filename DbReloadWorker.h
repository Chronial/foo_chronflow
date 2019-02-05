#pragma once
#include "Helpers.h"
#include "DbAlbumCollection.h"

class EngineThread;

class DbReloadWorker {
	metadb_handle_list library;
	EngineThread& engineThread;
	std::thread thread;
	std::promise<void> copyDone;
	std::atomic<bool> kill = false;

public:
	explicit DbReloadWorker(EngineThread& engineThread);
	DbReloadWorker(DbReloadWorker&) = delete;
	DbReloadWorker(DbReloadWorker&&) = delete;
	DbReloadWorker& operator=(DbReloadWorker&) = delete;
	DbReloadWorker& operator=(DbReloadWorker&&) = delete;
	~DbReloadWorker();
	DbAlbums albums;
	service_ptr_t<titleformat_object> albumMapper;

private:
	void threadProc();
	void generateData();
};