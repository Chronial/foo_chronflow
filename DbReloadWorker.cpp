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

static const DBUngroupedParams plparams;
DbReloadWorker::~DbReloadWorker() {
  abort.set();
  try {
    // interrupt waiting for copy
    copyDone.set_value();
  } catch (std::future_error&) {
  }  // copy was already done
  shared_selection.reset();
  //debug t_size d = shared_selection.use_count();

  if (thread.joinable())
    thread.join();
}

void DbReloadWorker::threadProc() {
  TRACK_CALL_TEXT("DbReloadWorker::threadProc");
  pfc::hires_timer timer;

  timer.start();
  t_size selection_count = 0;
  try {
    if (shared_selection.use_count() > 0)
      selection_count = shared_selection->get_count();
  }
  catch (std::exception& e) {
  }

  const bool bwholelibrary = configData->IsWholeLibrary();

  //std::string filter, group, sort, albumtitle;
  pfc::string8 filter, group, sort, albumtitle;
  bool sortgroup;

  if (!configData->SourceLibrarySelector)
  {
    shared_selection.reset();
    selection_count = 0;
  }

  if (configData->IsWholeLibrary() || selection_count > 0) {
    filter = configData->Filter;
    group = configData->Group;
    sortgroup = configData->SortGroup;
    sort = configData->SortGroup? "" : configData->Sort;
    albumtitle = configData->AlbumTitle;
  } else {
    filter = plparams.filter;
    if (configData->SourcePlaylistGroup) {
      group = configData->Group;
      albumtitle = configData->AlbumTitle;
    } else {
      group = plparams.group;
      group += "|$hi()";
      albumtitle = configData->SourcePlaylistNGTitle;
    }
    sortgroup = plparams.sortgroup;
    sort = plparams.sort;
  }

  engineThread.runInMainThread([&] {
    //abort.check();
    ++engineThread.libraryVersion;

    db = make_unique<db_structure::DB>(
        engineThread.libraryVersion, filter.c_str(), group.c_str(), sort.c_str() , albumtitle.c_str());

    if (selection_count > 0) {
      library = *shared_selection;
      //debug t_size shr_count = shared_selection.use_count();
    }
    else
    if (configData->IsWholeLibrary()) {
      // copy whole library
      library_manager::get()->get_all_items(library);
    } else {
      // playlist covers
      const t_size playlist =
          configData->FindSourcePlaylist(PlSrcFilter::ANY_PLAYLIST);
      playlist_manager::get()->playlist_get_all_items(playlist, library);
    }
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
