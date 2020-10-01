#pragma once

#include "cover_positions_compiler.h"
#include "ConfigGuids.h"
#include "ConfigCoverConfigs.h"

namespace coverflow {

class cfg_compiledCPInfoPtr : public cfg_var {

 private:
  int list_position;
  shared_ptr<CompiledCPInfo> data_;
  friend class configData;

 public:

  void pub_get_data_raw(stream_writer* p_stream, abort_callback& p_abort) {
    get_data_raw(p_stream, p_abort);
  }
  void pub_set_data_raw(stream_reader* p_stream, t_size p_sizehint,
                        abort_callback& p_abort) {
    set_data_raw(p_stream, 0, p_abort);
  }

 protected:
  void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) final;
  void set_data_raw(stream_reader* p_stream, t_size p_sizehint,
                    abort_callback& p_abort) final;

 public:
#pragma warning(push)
#pragma warning(disable: 4996)
//#define _CXX20_DEPRECATE_OLD_SHARED_PTR_ATOMIC_SUPPORT
//#define _SILENCE_CXX20_OLD_SHARED_PTR_ATOMIC_SUPPORT_DEPRECATION_WARNING

  cfg_compiledCPInfoPtr() : cfg_var(guid_CompiledCPInfoPtr) {
    data_ = nullptr;
    list_position = -1;
  }
  std::pair <int, shared_ptr<CompiledCPInfo>> get() const {
    if (this->data_) {
      return {list_position, std::atomic_load(&this->data_)};
    } else {
      return {list_position, nullptr};
    }
  }
  void set(int listposition, shared_ptr<CompiledCPInfo> value) {
    list_position = listposition;
    std::atomic_store(&this->data_, std::move(value));
  }
#pragma warning(pop)

  void reset() {
    pfc::string8 defstr(defaultCoverConfig);
    list_position = GetCoverConfigPosition(builtInCoverConfigs(), defstr);
    std::atomic_store(&this->data_, {});
  }
  inline explicit cfg_compiledCPInfoPtr(const GUID& p_guid)
    : cfg_var(p_guid) {
  }
};
} // namespace coverflow
