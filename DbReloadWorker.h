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
  shared_ptr<metadb_handle_list> shared_selection;
 public:
  //void SetAbort() { abort.set(); }
  explicit DbReloadWorker(EngineThread& engineThread)
      : engineThread(engineThread), thread(catchThreadExceptions("DBReloadWorker", [&] {
          this->threadProc();
        })) {

    if (engineThread.libraryVersion > 0) {
    }
    SetThreadPriority(thread.native_handle(), THREAD_PRIORITY_BELOW_NORMAL);
    SetThreadPriorityBoost(thread.native_handle(), TRUE);
  };
  DbReloadWorker(EngineThread& engineThread, shared_ptr<metadb_handle_list> sharedselection)
      : engineThread(engineThread), shared_selection(sharedselection), thread(catchThreadExceptions("DBReloadWorker", [&] {
      this->threadProc();
          })) {

      if (engineThread.libraryVersion > 0) {
      }
      SetThreadPriority(thread.native_handle(), THREAD_PRIORITY_BELOW_NORMAL);
      SetThreadPriorityBoost(thread.native_handle(), TRUE);
  };

  NO_MOVE_NO_COPY(DbReloadWorker);
  ~DbReloadWorker();

  unique_ptr<::db::db_structure::DB> db;
  std::atomic<bool> completed = false;

 private:
  void threadProc();

  // Needs to be the last member so the others are initialized when the thread starts
  std::thread thread;
};
}  // namespace db
