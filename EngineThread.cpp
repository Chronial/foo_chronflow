#include "EngineThread.h"

#include "Engine.h"
#include "EngineWindow.h"
#include "Helpers.h"

void EngineThread::run() {
  TRACK_CALL_TEXT("Chronflow EngineThread");
  CoInitializeScope com_enable{};  // Required for compiling CoverPos Scripts
  engineWindow.makeContextCurrent();
  Engine engine(*this, engineWindow);
  engine.mainLoop();
}

void EngineThread::sendMessage(unique_ptr<engine_messages::Message>&& msg) {
  messageQueue.push(std::move(msg));
}

EngineThread::EngineThread(EngineWindow& engineWindow)
    : engineWindow(engineWindow), thread(&EngineThread::run, this) {
  instances.insert(this);
  play_callback_reregister(flag_on_playback_new_track, true);
}

EngineThread::~EngineThread() {
  instances.erase(this);
  this->send<EM::StopThreadMessage>();
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

void EngineThread::on_playback_new_track(metadb_handle_ptr p_track) {
  send<EM::PlaybackNewTrack>(p_track);
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
    if (!*deadPtr) {
      f();
    }
  });
}
