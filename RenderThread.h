#pragma once
#include "TextureCache.h"
#include "DisplayPosition.h"
#include "DbAlbumCollection.h"
#include "DbReloadWorker.h"
#include "BlockingQueue.h"
#include "Renderer.h"

class AppInstance;
class DbReloadWorker;

template<typename T, typename... _Types>
static std::tuple<std::unique_ptr<T>, std::future<typename T::ValueType>> build_msg(_Types&&... _Args) {
	const unique_ptr<T> msg = make_unique<T>(std::forward<_Types>(_Args)...);
	const std::future<typename T::ValueType> future = msg->promise.get_future();
	return make_tuple(std::move(msg), std::move(future));
}



class RenderThread {
public:
	class Message {
	public:
		virtual ~Message() {};
		virtual void execute(RenderThread&) {};
	};

	template <typename T>
	class AnswerMessage : public Message {
	public:
		typedef T ValueType;
		std::promise<T> promise;
	};

	class PaintMessage : public Message {};
	class StopThreadMessage : public Message {};
	class InitDoneMessage : public AnswerMessage <bool> {};
	class RedrawMessage : public Message { void execute(RenderThread&); };
	class TextFormatChangedMessage : public Message { void execute(RenderThread&); };
	class DeviceModeMessage : public Message { void execute(RenderThread&); };
	class WindowResizeMessage : public Message {
	public:
		WindowResizeMessage(int width, int height) : width(width), height(height) {};
		void execute(RenderThread&);
		const int width;
		const int height;
	};
	class TargetChangedMessage : public Message { void execute(RenderThread&); };

	class GetPosAtCoordsMessage : public AnswerMessage <std::optional<CollectionPos>> {
	public:
		GetPosAtCoordsMessage(int x, int y) : x(x), y(y) {};
		void execute(RenderThread&);
		const int x;
		const int y;
	};

	class ChangeCPScriptMessage : public Message {
	public:
		ChangeCPScriptMessage(const pfc::string_base &script) : script(script) {};
		void execute(RenderThread&);
		const pfc::string8 script;
	};

	class CollectionReloadedMessage : public Message { void execute(RenderThread&); };
	class WindowHideMessage : public Message { void execute(RenderThread&); };
	class WindowShowMessage : public Message { void execute(RenderThread&); };



private:
	DisplayPosition displayPos;
	TextureCache texCache;
	Renderer renderer;
	AppInstance* appInstance;
public:
	RenderThread(AppInstance* appInstance);
	~RenderThread();

	void sendMessage(unique_ptr<Message>&& msg);

	template<typename T, typename... _Types>
	std::future<typename T::ValueType> sendSync(_Types&&... _Args) {
		unique_ptr<T> msg = make_unique<T>(std::forward<_Types>(_Args)...);
		std::future<typename T::ValueType> future = msg->promise.get_future();
		sendMessage(std::move(msg));
		return std::move(future);
	}
	template<typename T, typename... _Types>
	void send(_Types&&... _Args) {
		sendMessage(make_unique<T>(std::forward<_Types>(_Args)...));
	}
private:
	int timerResolution;
	bool timerInPeriod;
	int refreshRate;
	double afterLastSwap;

	void updateRefreshRate();

	static unsigned int WINAPI runRenderThread(void* lpParameter);
	void threadProc();
	void onPaint();
	bool doPaint;

	HANDLE renderThread;

	BlockingQueue<unique_ptr<Message>> messageQueue;
};