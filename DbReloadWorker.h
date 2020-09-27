#pragma once
// clang-format off
#include "DbAlbumCollection.h"
#include "EngineThread.h"
// clang-format on
#include "pfc/synchro_win.h"
#include "utils.h"

namespace db {

using engine::EngineThread;

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
}  // namespace db
