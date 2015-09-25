#pragma once
#include "Helpers.h"
#include "DbAlbumCollection.h"

class AppInstance;

class DbReloadWorker {
	metadb_handle_list library;
	AppInstance *const appInstance;
	HANDLE thread = nullptr;
	std::atomic<bool> kill = false;
	DbReloadWorker(AppInstance* instance);

public:
	DbAlbums albums;
	service_ptr_t<titleformat_object> albumMapper;

	// call only from mainthread
	static void startNew(AppInstance* instance);
	~DbReloadWorker();

private:
	void threadProc();
	void generateData();
};