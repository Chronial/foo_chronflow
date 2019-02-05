#pragma once
#include "BlockingQueue.h"

class RenderWindow;

namespace engine_messages {
struct Message;
}

class CallbackHolder {
	struct Data {
		bool dead = false;
		std::shared_mutex mutex;
	};
	std::shared_ptr<Data> data;

public:
	CallbackHolder();
	~CallbackHolder() noexcept;
	void addCallback(std::function<void()> f);
};

class RenderThread : public play_callback_impl_base {
public:
	explicit RenderThread(RenderWindow& renderWindow);
	RenderThread(RenderThread&) = delete;
	RenderThread& operator=(RenderThread&) = delete;
	RenderThread(RenderThread&&) = delete;
	RenderThread& operator=(RenderThread&&) = delete;
	~RenderThread();

	void sendMessage(unique_ptr<engine_messages::Message>&& msg);

	/// Sends a message and get a future to wait for the response.
	/// You should only wait in the mainthread, as waiting in other threads
	/// will deadlock if the Engine is shut down while you're waiting.
	template<typename T, typename... _Types>
	std::future<typename T::ValueType> sendSync(_Types&&... _Args) {
		unique_ptr<T> msg = make_unique<T>(std::forward<_Types>(_Args)...);
		std::future<typename T::ValueType> future = msg->promise.get_future();
		sendMessage(std::move(msg));
		return std::move(future);
	};

	template<typename T, typename... _Types>
	void send(_Types&&... _Args) {
		sendMessage(make_unique<T>(std::forward<_Types>(_Args)...));
	};

	void invalidateWindow();

	void on_playback_new_track(metadb_handle_ptr p_track);

	/// Runs a function in the foobar2000 mainthread
	/// Guarantees that the callback runs only if this thread is still alive.
	void runInMainThread(std::function<void()> f);

	static void forEach(std::function<void(RenderThread&)>);

private:
	void run();
	RenderWindow& renderWindow;
	BlockingQueue<unique_ptr<engine_messages::Message>> messageQueue;
	CallbackHolder callbackHolder;
	std::thread thread;

	static std::unordered_set<RenderThread*> instances;
	friend class Engine;
};