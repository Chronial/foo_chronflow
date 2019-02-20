#include "DbReloadWorker.h"

#include "Engine.h"
#include "EngineThread.h"
#include "config.h"
#include "utils.h"

DbReloadWorker::DbReloadWorker(EngineThread& engineThread)
    : engineThread(engineThread),
      thread(catchThreadExceptions("DBReloadWorker", [&] { this->threadProc(); })) {
  SetThreadPriority(thread.native_handle(), THREAD_PRIORITY_BELOW_NORMAL);
  SetThreadPriorityBoost(thread.native_handle(), TRUE);
};

DbReloadWorker::~DbReloadWorker() {
  abort.set();
  try {
    // interrupt waiting for copy
    copyDone.set_value();
  } catch (std::future_error&) {
  }  // copy was already done
  if (thread.joinable())
    thread.join();
}

void DbReloadWorker::threadProc() {
  TRACK_CALL_TEXT("Chronflow DbReloadWorker");

  engineThread.runInMainThread([&] {
    ++engineThread.libraryVersion;
    db = make_unique<db_structure::DB>(
        engineThread.libraryVersion, cfgFilter.c_str(), cfgGroup.c_str(),
        (cfgSortGroup ? "" : cfgSort.c_str()), cfgAlbumTitle.c_str());
    // copy whole library
    library_manager::get()->get_all_items(library);
    try {
      copyDone.set_value();
    } catch (std::future_error&) {
    }
  });

  copyDone.get_future().wait();
  abort.check();

  {
    console::timer_scope timer("foo_chronflow collection generated in");

    DBWriter writer(*db);
    writer.add_tracks(std::move(library), abort);
  }
  abort.check();

  engineThread.send<EM::CollectionReloadedMessage>();
};
