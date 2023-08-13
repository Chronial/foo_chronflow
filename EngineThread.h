#pragma once
// clang-format off
#include "EngineWindow.fwd.h"
#include "PlaybackTracer.fwd.h"
#include "PlaylistCallback.h"
#include "BlockingQueue.h"
#include "style_manager.h"
#include "utils.h"

// x ui
#include "ConfigData.h"
// clang-format on

namespace engine_messages {
struct Message;
}

namespace engine {

using render::StyleManager;

class CallbackHolder {
  std::shared_ptr<bool> deadPtr;

 public:
  CallbackHolder();
  CallbackHolder(const CallbackHolder&) = delete;
  CallbackHolder& operator=(const CallbackHolder&) = delete;
  CallbackHolder(CallbackHolder&&) = delete;
  CallbackHolder& operator=(CallbackHolder&&) = delete;
  ~CallbackHolder() noexcept;
  void addCallback(std::function<void()> f);
};

class EngineThread : public play_callback_impl_base,
                     public library_callback_dynamic_impl_base,
                     public PlaylistCallback {
 public:
  explicit EngineThread(EngineWindow& engineWindow, StyleManager& styleManager);
  NO_MOVE_NO_COPY(EngineThread);
  ~EngineThread();

  void sendMessage(unique_ptr<::engine_messages::Message>&& msg);

  /// Sends a message and get a future to wait for the response.
  /// You should only wait in the mainthread, as waiting in other threads
  /// will deadlock if the Engine is shut down while you're waiting.
  template <typename T, typename... _Types>
  std::future<typename T::ValueType> sendSync(_Types&&... _Args) {
    unique_ptr<T> msg = make_unique<T>(std::forward<_Types>(_Args)...);
    std::future<typename T::ValueType> future = msg->promise.get_future();
    sendMessage(std::move(msg));
    return std::move(future);
  };

  template <typename T, typename... _Types>
  void send(_Types&&... _Args) {
    sendMessage(make_unique<T>(std::forward<_Types>(_Args)...));
  };

  //serves playlistcallback sendmessage
  HWND EngineThread::GetEngineWindowWnd();
  //
  size_t GetDispFlags();
  bool GetCoverDispFlagU(DispFlags flg);
  void SetCoverDispFlagU(DispFlags flg, bool val);
  bool IsWholeLibrary();
  t_size FindSourcePlaylist(PlSrcFilter mode) const;
  //serves playback callback engine thread
  bool IsSourcePlaylistOn(t_size playlist, PlSrcFilter mode);
  void GetState(src_state& srcstate, bool init = true);
  void GetPlaylistSource(pfc::string& out, bool activesource, bool bguid);
  void SetPlaylistSource(pfc::string out, bool activesource, bool bguid);
  //..
  void invalidateWindow();

  void on_style_change();

  void on_playback_new_track(metadb_handle_ptr p_track) final;
  // engine
  void on_items_added(metadb_handle_list_cref p_data) final;
  void on_items_removed(metadb_handle_list_cref p_data) final;
  void on_items_modified(metadb_handle_list_cref p_data) final;

  /// Runs a function in the foobar2000 mainthread
  /// Guarantees that the callback runs only if this thread is still alive.
  void runInMainThread(std::function<void()> f);

  static void forEach(std::function<void(EngineThread&)>);

  /// Library version tag – only access from mainthread
  t_uint64 libraryVersion{0};

 protected:
  const metadb_handle_list_ref getEngineWindowSelection();

 private:
  void run();
  EngineWindow& engineWindow;
  StyleManager& styleManager;
  BlockingQueue<unique_ptr<::engine_messages::Message>> messageQueue;
  CallbackHolder callbackHolder;
  std::thread thread;

  static std::unordered_set<EngineThread*> instances;
  friend class Engine;
  friend class PlaybackTracer;
};
} // namespace
