#include "DbReloadWorker.h"

#include "Engine.h"
#include "EngineThread.h"
#include "config.h"
#include "utils.h"

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

  auto& keyIndex = albums.get<0>();
  {
    compiler->compile_safe_ex(keyBuilder, cfgGroup);
    albums.reserve(library.get_size());
    service_ptr_t<titleformat_object> sortFormatter;
    if (!cfgSortGroup) {
      compiler->compile_safe(sortFormatter, cfgSort);
    }
    pfc::string8_fast_aggressive keyBuffer;
    pfc::string8_fast_aggressive sortBuffer;
    pfc::string8_fast_aggressive titleBuffer;
    pfc::stringcvt::string_wide_from_utf8_fast sortBufferWide;

    for (const metadb_handle_ptr& track : library) {
      if (kill)
        return;

      track->format_title(nullptr, keyBuffer, keyBuilder, nullptr);
      auto album = keyIndex.find(keyBuffer.get_ptr());
      if (album == keyIndex.end()) {
        if (cfgSortGroup) {
          sortBufferWide.convert(keyBuffer);
        } else {
          track->format_title(nullptr, sortBuffer, sortFormatter, nullptr);
          sortBufferWide.convert(sortBuffer);
        }

        track->format_title(nullptr, titleBuffer, cfgAlbumTitleScript, nullptr);

        bool success;
        std::tie(album, success) = albums.emplace(keyBuffer, sortBufferWide, titleBuffer);
      }
      album->tracks.add_item(track);
    }
  }
  library.remove_all();
};
