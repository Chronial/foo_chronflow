// clang-format off
#include "DbReloadWorker.h"

#include "ConfigData.h"
#include "DbAlbumCollection.h"
#include "Engine.h"
// clang-format on
namespace db {

using coverflow::configData;
using engine::EngineThread;
using EM = engine::Engine::Messages;

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
  TRACK_CALL_TEXT("DbReloadWorker::threadProc");
  pfc::hires_timer timer;
  timer.start();

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

  DBWriter(*db).add_tracks(std::move(library), abort);
  abort.check();

  FB2K_console_formatter() << AppNameInternal << " collection generated in: "
                           << pfc::format_time_ex(timer.query(), 6);
  completed = true;
  engineThread.send<EM::CollectionReloadedMessage>();
};
}  // namespace db
