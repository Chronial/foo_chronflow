#pragma once
#include "base.h"

struct CoverConfig {
  std::string script;
  bool buildIn = false;
};

using CoverConfigMap = std::map<std::string, CoverConfig, ILessUtf8>;

class cfg_coverConfigs : public cfg_var, public CoverConfigMap {
 public:
  explicit cfg_coverConfigs(const GUID& p_guid);

 protected:
  void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) final;
  void set_data_raw(stream_reader* p_stream, t_size p_sizehint,
                    abort_callback& p_abort) final;

 private:
  static const unsigned int version = 1;
};
