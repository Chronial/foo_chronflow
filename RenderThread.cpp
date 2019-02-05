#include "stdafx.h"
#include "Helpers.h"

#include "RenderThread.h"
#include "Engine.h"
#include "RenderWindow.h"



void RenderThread::run(){
	TRACK_CALL_TEXT("Chronflow RenderThread");
	CoInitializeScope com_enable{}; // Required for compiling CoverPos Scripts
	renderWindow.makeContextCurrent();
	Engine engine(*this, renderWindow);
	engine.mainLoop();
}

void RenderThread::sendMessage(unique_ptr<engine_messages::Message>&& msg){
	messageQueue.push(std::move(msg));
}


RenderThread::RenderThread(RenderWindow& renderWindow) : 
	renderWindow(renderWindow),
	thread(&RenderThread::run, this)
{
	instances.insert(this);
	play_callback_reregister(flag_on_playback_new_track, true);
}

RenderThread::~RenderThread(){
	instances.erase(this);
	this->send<EM::StopThreadMessage>();
	if (thread.joinable())
		thread.join();
}

void RenderThread::invalidateWindow() {
	RedrawWindow(renderWindow.hWnd, NULL, NULL, RDW_INVALIDATE);
}

std::unordered_set<RenderThread*> RenderThread::instances{};

void RenderThread::forEach(std::function<void(RenderThread&)> f){
	for (auto instance : instances) {
		f(*instance);
	}
}

void RenderThread::on_playback_new_track(metadb_handle_ptr p_track)
{
	send<EM::PlaybackNewTrack>(p_track);
}

void RenderThread::runInMainThread(std::function<void()> f) {
	callbackHolder.addCallback(f);
}

CallbackHolder::CallbackHolder() : data(make_shared<Data>()) {}

CallbackHolder::~CallbackHolder() noexcept {
	std::unique_lock<std::shared_mutex> lock(data->mutex);
	data->dead = true;
}

void CallbackHolder::addCallback(std::function<void()> f) {
	fb2k::inMainThread([f, data = this->data]{
		std::shared_lock<std::shared_mutex> lock(data->mutex);
		if (!data->dead) {
			f();
		}
	 });
}
