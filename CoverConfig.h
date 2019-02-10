#pragma once

struct CoverConfig {
  pfc::string8 name;
  pfc::string8 script;
  bool buildIn = false;
};

class cfg_coverConfigs : public cfg_var, public pfc::list_t<CoverConfig> {
 public:
  explicit cfg_coverConfigs(const GUID& p_guid);
  CoverConfig* getPtrByName(const char* name);
  bool removeItemByName(const char* name);
  void sortByName();

 protected:
  void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) final;
  void set_data_raw(stream_reader* p_stream, t_size p_sizehint,
                    abort_callback& p_abort) final;

 private:
  static const unsigned int version = 1;
};
