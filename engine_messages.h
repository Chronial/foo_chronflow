#pragma once
#include "stdafx.h"
#include "DbAlbumCollection.h"

class Engine;

namespace engine_messages {

struct Message {
public:
	virtual ~Message() {};
	virtual void execute(Engine&) {};
};

template <typename... Types>
struct DataMessage : Message {
	DataMessage(Types... Args) { data = make_tuple(Args...); }
	void execute(Engine& e) override {
		std::apply(&DataMessage::run, std::tuple_cat(std::tie(*this, e), data));
	}
	virtual void run(Engine&, Types...) {};
	std::tuple<Types...> data;
};

template <typename T, typename... Types>
struct AnswerMessage : Message {
	typedef T ValueType;
	AnswerMessage(Types... Args) { data = make_tuple(Args...); }
	void execute(Engine& e) override {
		promise.set_value(
			std::apply(&AnswerMessage::run, std::tuple_cat(std::tie(*this, e), data)));
	}
	virtual T run(Engine& e, Types...) = 0;
	std::promise<T> promise;
	std::tuple<Types...> data;
};

}


#define E_MSG(NAME, ...) struct NAME : public engine_messages::DataMessage<__VA_ARGS__> { \
	using DataMessage::DataMessage; \
	void run(Engine&, __VA_ARGS__) final; \
}

#define E_ANSWER_MSG(NAME, T, ...)  struct NAME : public engine_messages::AnswerMessage<T, __VA_ARGS__> { \
	using AnswerMessage::AnswerMessage; \
	T run(Engine&, __VA_ARGS__) final; \
}


struct Engine::Messages {
	class PaintMessage : public engine_messages::Message {};
	class StopThreadMessage : public engine_messages::Message {};

	E_MSG(RedrawMessage);
	E_MSG(TextFormatChangedMessage);
	E_MSG(DeviceModeMessage);
	E_MSG(CharEntered, WPARAM);
	E_MSG(TargetChangedMessage);
	E_MSG(MoveToNowPlayingMessage);
	E_MSG(ReloadCollection);
	E_MSG(CollectionReloadedMessage);
	E_MSG(WindowHideMessage);
	E_MSG(WindowShowMessage);
	E_MSG(WindowResizeMessage, int, int);
	E_MSG(MoveTargetMessage, int, bool);
	E_MSG(MoveToAlbumMessage, std::string);
	E_MSG(MoveToCurrentTrack, metadb_handle_ptr);
	E_MSG(ChangeCPScriptMessage, pfc::string8);
	E_ANSWER_MSG(GetAlbumAtCoords, std::optional<AlbumInfo>, int, int);
	E_ANSWER_MSG(GetTargetAlbum, std::optional<AlbumInfo>);
	E_MSG(Run, std::function<void()>);
	E_MSG(PlaybackNewTrack, metadb_handle_ptr);
};

#undef E_MSG
#undef E_ANSWER_MSG