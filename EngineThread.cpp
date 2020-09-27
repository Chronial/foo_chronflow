// clang-format off
#include "EngineThread.h"

#include "ContainerWindow.h"
#include "Engine.h"
#include "EngineWindow.h"
#include "helpers\win32_misc.h"
#include "utils.h"
// clang-format on

namespace engine {

using EM = engine::Engine::Messages;

void EngineThread::run() {
  TRACK_CALL_TEXT("foo_chronflow EngineThread");
  CoInitializeScope com_enable{};
  engineWindow.makeContextCurrent();

  // We can only destroy the Engine while EngineThread is beeing destroyed,
  // since mainthread callbacks held by EngineThread might reference Engine objects.
  std::optional<Engine> engine;
  try {
    engine.emplace(*this, engineWindow, styleManager);
    engine->mainLoop();
  } catch (std::exception& e) {
    runInMainThread([&] {
      engineWindow.container.destroyEngineWindow((PFC_string_formatter()
                                                  << "Engine died with exception:\n"
                                                  << e.what())
                                                     .c_str());
    });
    while (nullptr == dynamic_cast<EM::StopThread*>(messageQueue.pop().get())) {
      // Going through the message queue has two purposes:
      // 1. We wait for and find StopThread messages
      // 2. We abandon any promises that are enqueued
    }
  }
}

void EngineThread::sendMessage(unique_ptr<engine_messages::Message>&& msg) {
  messageQueue.push(std::move(msg));
}

EngineThread::EngineThread(EngineWindow& engineWindow, StyleManager& styleManager)
    : engineWindow(engineWindow), styleManager(styleManager) {
  instances.insert(this);
  styleManager.setChangeHandler([&] { this->on_style_change(); });
  play_callback_reregister(flag_on_playback_new_track, true);
  std::packaged_task<void(EngineThread*)> task(&EngineThread::run);
  thread = std::thread(std::move(task), this);
}

EngineThread::~EngineThread() {
  instances.erase(this);
  styleManager.setChangeHandler([] {});
  this->send<EM::StopThread>();
  // If the shutdown causes an exception, we need a second
  // message to shut down the exception handler.
  this->send<EM::StopThread>();
  if (thread.joinable())
    thread.join();
}

void EngineThread::invalidateWindow() {
  RedrawWindow(engineWindow.hWnd, nullptr, nullptr, RDW_INVALIDATE);
}

std::unordered_set<EngineThread*> EngineThread::instances{};

void EngineThread::forEach(std::function<void(EngineThread&)> f) {
  for (auto instance : instances) {
    f(*instance);
  }
}

void EngineThread::on_style_change() {
  send<EM::TextFormatChangedMessage>();
  invalidateWindow();
}

void EngineThread::on_playback_new_track(metadb_handle_ptr p_track) {
  send<EM::PlaybackNewTrack>(p_track);
}
void EngineThread::on_items_added(metadb_handle_list_cref p_data) {
  send<EM::LibraryItemsAdded>(p_data, libraryVersion);
}
void EngineThread::on_items_removed(metadb_handle_list_cref p_data) {
  send<EM::LibraryItemsRemoved>(p_data, libraryVersion);
}
void EngineThread::on_items_modified(metadb_handle_list_cref p_data) {
  send<EM::LibraryItemsModified>(p_data, libraryVersion);
}

void EngineThread::runInMainThread(std::function<void()> f) {
  callbackHolder.addCallback(f);
}

CallbackHolder::CallbackHolder() : deadPtr(make_shared<bool>(false)) {}

CallbackHolder::~CallbackHolder() noexcept {
  PFC_ASSERT(core_api::is_main_thread());
  *deadPtr = true;
}

void CallbackHolder::addCallback(std::function<void()> f) {
  fb2k::inMainThread([f, deadPtr = this->deadPtr] {
    TRACK_CALL_TEXT("foo_chronflow::inMainThread");
    if (!*deadPtr) {
      f();
    }
  });
}
}  // namespace engine
