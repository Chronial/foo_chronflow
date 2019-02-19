#pragma once
#include "DbAlbumCollection.h"

class EngineThread;

class DbReloadWorker {
  metadb_handle_list library;
  EngineThread& engineThread;
  std::promise<void> copyDone;
  std::atomic<bool> kill = false;
  std::thread thread;

 public:
  explicit DbReloadWorker(EngineThread& engineThread);
  DbReloadWorker(DbReloadWorker&) = delete;
  DbReloadWorker(DbReloadWorker&&) = delete;
  DbReloadWorker& operator=(DbReloadWorker&) = delete;
  DbReloadWorker& operator=(DbReloadWorker&&) = delete;
  ~DbReloadWorker();
  db_structure::DB albums;
  service_ptr_t<titleformat_object> keyBuilder;

 private:
  void threadProc();
  void generateData();
};
