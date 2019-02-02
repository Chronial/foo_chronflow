#pragma once
#include "TextureCache.h"
#include "DisplayPosition.h"
#include "DbAlbumCollection.h"
#include "DbReloadWorker.h"
#include "BlockingQueue.h"
#include "Renderer.h"
#include "FindAsYouType.h"

class AppInstance;
class DbReloadWorker;

#define RT_MSG(NAME, ...) struct NAME : public DataMessage<__VA_ARGS__> { \
	using DataMessage::DataMessage; \
	void run(RenderThread&, __VA_ARGS__); \
}

#define RT_ANSWER_MSG(NAME, T, ...)  struct NAME : public AnswerMessage<T, __VA_ARGS__> { \
	using AnswerMessage::AnswerMessage; \
	T run(RenderThread&, __VA_ARGS__); \
}

class RenderThread {
public:
	class Message {
	public:
		virtual ~Message() {};
		virtual void execute(RenderThread&) {};
	};

	template <typename... Types>
	class DataMessage : public Message {
	public:
		DataMessage(Types... Args) { data = make_tuple(Args...); }
		virtual void run(RenderThread&, Types...) {};
		void execute(RenderThread& rt) {
			std::apply(&DataMessage::run, std::tuple_cat(std::tie(*this, rt), data));
		}
		std::tuple<Types...> data;
	};

	template <typename T, typename... Types>
	class AnswerMessage : public Message {
	public:
		typedef T ValueType;
		AnswerMessage(Types... Args) { data = make_tuple(Args...); }
		virtual T run(RenderThread& rt, Types...) = 0;
		void execute(RenderThread& rt) {
			promise.set_value(
				std::apply(&AnswerMessage::run, std::tuple_cat(std::tie(*this, rt), data)));
		}
		std::promise<T> promise;
		std::tuple<Types...> data;
	};

	class PaintMessage : public Message {};
	class StopThreadMessage : public Message {};

	class InitDoneMessage : public AnswerMessage <bool> {
		void execute(RenderThread&) {};
		bool run(RenderThread&) { return false; };
	};

	RT_MSG(RedrawMessage);
	RT_MSG(TextFormatChangedMessage);
	RT_MSG(DeviceModeMessage);
	RT_MSG(CharEntered, WPARAM);
	RT_MSG(TargetChangedMessage);
	RT_MSG(MoveToNowPlayingMessage);
	RT_MSG(ReloadCollectionMessage);
	RT_MSG(CollectionReloadedMessage);
	RT_MSG(WindowHideMessage);
	RT_MSG(WindowShowMessage);
	RT_MSG(WindowResizeMessage, int, int);
	RT_MSG(MoveTargetMessage, int, bool);
	RT_MSG(MoveToAlbumMessage, std::string);
	RT_MSG(MoveToTrack, metadb_handle_ptr);
	RT_MSG(ChangeCPScriptMessage, pfc::string8);
	RT_ANSWER_MSG(GetAlbumAtCoords, std::optional<AlbumInfo>, int, int);
	RT_ANSWER_MSG(GetTargetAlbum, std::optional<AlbumInfo>);
	RT_MSG(Run, std::function<void()>);


private:
	FindAsYouType findAsYouType;
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