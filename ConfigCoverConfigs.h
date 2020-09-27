#pragma once
#include "utils.h"

namespace coverflow {
struct CoverConfig {
  std::string script;
  bool buildIn = false;
};

using CoverConfigMap = std::map<std::string, CoverConfig, ILessUtf8>;

CoverConfigMap builtInCoverConfigs();

int GetCoverConfigPosition(CoverConfigMap configs, pfc::string8 name);

pfc::string8 GetCoverConfigScript(CoverConfigMap configs, pfc::string8 name);

class cfg_coverConfigs : public cfg_var, public CoverConfigMap {
 public:
  explicit cfg_coverConfigs(const GUID& p_guid)
    : cfg_var(p_guid), CoverConfigMap(builtInCoverConfigs()) {
  }
  void pub_get_data_raw(stream_writer* p_stream, abort_callback& p_abort) {
    get_data_raw(p_stream, p_abort);
  }
  void pub_set_data_raw(stream_reader* p_stream, t_size p_sizehint,
                        abort_callback& p_abort) {
    set_data_raw(p_stream, 0, p_abort);
  }

 private:

  void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) final;
  void set_data_raw(stream_reader* p_stream, t_size p_sizehint,
                    abort_callback& p_abort) final;

 private:
  static const unsigned int version = 2;
};
} // namespace
