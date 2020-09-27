#include "ConfigCompiledInfo.h"

namespace coverflow {
void cfg_compiledCPInfoPtr::set_data_raw(stream_reader* p_stream, t_size /*p_sizehint*/,
                                         abort_callback& p_abort) {
  p_stream->read_lendian_t(list_position, p_abort);
  bool configNotEmpty;
  p_stream->read_lendian_t(configNotEmpty, p_abort);
  if (configNotEmpty) {
    auto data = make_shared<CompiledCPInfo>();
    CompiledCPInfo::unserialize(*data, p_stream, p_abort);
    this->set(list_position, data);
  } else {
    this->reset();
  }
}

void cfg_compiledCPInfoPtr::get_data_raw(stream_writer* p_stream,
                                         abort_callback& p_abort) {
  auto [position, data] = this->get();
  p_stream->write_lendian_t(list_position, p_abort);
  if (data) {
    p_stream->write_lendian_t(true, p_abort);
    data->serialize(p_stream, p_abort);
  } else {
    p_stream->write_lendian_t(false, p_abort);
  }
}
} // namespace foo_coverflow
