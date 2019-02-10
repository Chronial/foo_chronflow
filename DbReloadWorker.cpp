#include "DbReloadWorker.h"

#include "Engine.h"
#include "EngineThread.h"
#include "base.h"
#include "config.h"

DbReloadWorker::DbReloadWorker(EngineThread& engineThread)
    : engineThread(engineThread), thread(&DbReloadWorker::threadProc, this) {
  SetThreadPriority(thread.native_handle(), THREAD_PRIORITY_BELOW_NORMAL);
  SetThreadPriorityBoost(thread.native_handle(), TRUE);
};

DbReloadWorker::~DbReloadWorker() {
  kill = true;
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
    // copy whole library
    library_manager::get()->get_all_items(library);
    try {
      copyDone.set_value();
    } catch (std::future_error&) {
    }
  });
  copyDone.get_future().wait();
  if (kill)
    return;

  this->generateData();  // TODO: handle exception?
  if (kill)
    return;

  engineThread.send<EM::CollectionReloadedMessage>();
};

void DbReloadWorker::generateData() {
  console::timer_scope timer("foo_chronflow collection generated in");
  static_api_ptr_t<titleformat_compiler> compiler;
  static_api_ptr_t<metadb> db;

  if (!cfgFilter.is_empty()) {
    try {
      search_filter::ptr filter =
          static_api_ptr_t<search_filter_manager>()->create(cfgFilter);
      pfc::array_t<bool> mask;
      mask.set_size(library.get_count());
      filter->test_multi(library, mask.get_ptr());
      library.filter_mask(mask.get_ptr());
    } catch (pfc::exception const&) {
    };
  }

  if (kill)
    return;

  auto& groupIndex = albums.get<0>();
  {
    compiler->compile_safe_ex(albumMapper, cfgGroup);
    pfc::string8_fast_aggressive tmpSortString;
    albums.reserve(library.get_size());
    service_ptr_t<titleformat_object> sortFormatter;
    if (!cfgSortGroup) {
      compiler->compile_safe(sortFormatter, cfgSort);
    }
    for (t_size i = 0; i < library.get_size(); i++) {
      if (kill)
        return;
      pfc::string8_fast groupString;
      metadb_handle_ptr track = library.get_item(i);
      track->format_title(nullptr, groupString, albumMapper, nullptr);
      if (groupIndex.count(groupString.get_ptr()) == 0u) {
        std::wstring sortString;
        if (cfgSortGroup) {
          sortString = pfc::stringcvt::string_wide_from_utf8(groupString);
        } else {
          track->format_title(nullptr, tmpSortString, sortFormatter, nullptr);
          sortString = pfc::stringcvt::string_wide_from_utf8(tmpSortString);
        }

        pfc::string8_fast findAsYouType;
        track->format_title(nullptr, findAsYouType, cfgAlbumTitleScript, nullptr);

        metadb_handle_list tracks;
        tracks.add_item(track);
        albums.insert({groupString.get_ptr(), std::move(sortString),
                       std::move(findAsYouType), std::move(tracks)});
      } else {
        auto album = groupIndex.find(groupString.get_ptr());
        album->tracks.add_item(track);
      }
    }
  }
  library.remove_all();
};
