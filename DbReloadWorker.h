#pragma once
#include "DbAlbumCollection.h"
#include "utils.h"

class EngineThread;

class DbReloadWorker {
  metadb_handle_list library;
  EngineThread& engineThread;
  std::promise<void> copyDone;
  abort_callback_impl abort;

 public:
  explicit DbReloadWorker(EngineThread& engineThread);
  NO_MOVE_NO_COPY(DbReloadWorker);
  ~DbReloadWorker();

  unique_ptr<db_structure::DB> db;
  std::atomic<bool> completed = false;

 private:
  void threadProc();

  // Needs to be the last member so the others are initialized when the thread starts
  std::thread thread;
};
