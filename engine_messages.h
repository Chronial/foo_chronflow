#pragma once
// clang-format off
//#include "DbAlbumCollection.h"
//#include "engine.fwd.h"
#include "DbAlbumInfo.h"
#include "cover_positions_compiler.h"
#include "utils.h"
// clang-format on

namespace engine_messages {

using namespace engine;

struct Message {
 public:
  Message() = default;
  NO_MOVE_NO_COPY(Message);
  virtual ~Message() = default;
  virtual void execute(Engine&){};
};

template <typename... Types>
struct DataMessage : Message {
  explicit DataMessage(Types... Args) { data = std::make_tuple(Args...); }
  void execute(Engine& e) override {
    std::apply(&DataMessage::run, std::tuple_cat(std::tie(*this, e), data));
  }
  virtual void run(Engine&, Types...){};
  std::tuple<Types...> data;
};

template <typename T, typename... Types>
struct AnswerMessage : Message {
  using ValueType = T;
  explicit AnswerMessage(Types... Args) { data = std::make_tuple(Args...); }
  void execute(Engine& e) override {
    promise.set_value(
        std::apply(&AnswerMessage::run, std::tuple_cat(std::tie(*this, e), data)));
  }
  virtual T run(Engine& e, Types...) = 0;
  std::promise<T> promise;
  std::tuple<Types...> data;
};

} // namespace engine_messages

namespace engine {
//using namespace engine_messages;
using ::db::AlbumInfo;

#define E_MSG(NAME, ...) \
  struct NAME : public engine_messages::DataMessage<__VA_ARGS__> { \
    using DataMessage::DataMessage; \
    void run(Engine&, __VA_ARGS__) final; \
  }

#define E_ANSWER_MSG(NAME, T, ...) \
  struct NAME : public engine_messages::AnswerMessage<T, __VA_ARGS__> { \
    using AnswerMessage::AnswerMessage; \
    T run(Engine&, __VA_ARGS__) final; \
  }

struct Engine::Messages {
  E_MSG(StopThread);
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
  E_MSG(MoveToAlbumMessage, AlbumInfo);
  E_MSG(MoveToCurrentTrack, metadb_handle_ptr);
  E_MSG(ChangeCoverPositionsMessage, std::shared_ptr<CompiledCPInfo>);
  E_ANSWER_MSG(GetAlbumAtCoords, std::optional<AlbumInfo>, int, int);
  E_ANSWER_MSG(GetTargetAlbum, std::optional<AlbumInfo>);
  E_MSG(Run, std::function<void()>);
  E_MSG(PlaybackNewTrack, metadb_handle_ptr);
  E_MSG(LibraryItemsAdded, metadb_handle_list, t_uint64);
  E_MSG(LibraryItemsRemoved, metadb_handle_list, t_uint64);
  E_MSG(LibraryItemsModified, metadb_handle_list, t_uint64);
};

#undef E_MSG
#undef E_ANSWER_MSG

} // namespace engine
