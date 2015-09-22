#pragma once
#include "Helpers.h"
#include "DbAlbumCollection.h"

class DbReloadWorker;

class DbReloadWorker {
	metadb_handle_list library;
	AppInstance *const appInstance;
	DbReloadWorker(AppInstance* instance) : appInstance(instance){};

public:
	DbAlbums albums;
	service_ptr_t<titleformat_object> albumMapper;

	// call only from mainthread
	static void startNew(AppInstance* instance){
		(new DbReloadWorker(instance))->run();
	}

private:
	void run();
	void threadProc();
	void generateData();
};