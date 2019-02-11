#include "CoverConfig.h"

#include "utils.h"

const CoverConfigMap buildIn = [] {
  CoverConfigMap map;
  for (auto& [name, script] : builtInCoverConfigs) {
    map[name] = CoverConfig{script, true};
  }
  return map;
}();

cfg_coverConfigs::cfg_coverConfigs(const GUID& p_guid)
    : cfg_var(p_guid), CoverConfigMap(buildIn) {}

void cfg_coverConfigs::get_data_raw(stream_writer* p_stream, abort_callback& p_abort) {
  p_stream->write_lendian_t(version, p_abort);
  int customCount = std::count_if(
      begin(), end(), [](CoverConfigMap::value_type& x) { return !x.second.buildIn; });
  p_stream->write_lendian_t(customCount, p_abort);
  for (auto& [name, config] : *this) {
    if (config.buildIn)
      continue;
    p_stream->write_string(name.c_str(), p_abort);
    p_stream->write_string(config.script.c_str(), p_abort);
  }
}

void cfg_coverConfigs::set_data_raw(stream_reader* p_stream, t_size /*p_sizehint*/,
                                    abort_callback& p_abort) {
  int c, v;
  p_stream->read_lendian_t(v, p_abort);
  if (v != 1) {
    MessageBox(
        nullptr,
        L"Couldn't load cover configs (incompatible version)\r\nIf you have not done it "
        L"yet, kill foobar with the taskmanager and make a backup of your coverflow "
        L"configs with the version of foo_chronflow you created them with.",
        L"foo_chronflow - loading Error", MB_ICONERROR);
    return;
  }
  p_stream->read_lendian_t(c, p_abort);
  clear();
  for (int i = 0; i < c; i++) {
    pfc::string8 name;
    pfc::string8 script;
    p_stream->read_string(name, p_abort);
    p_stream->read_string(script, p_abort);
    insert({name.c_str(), CoverConfig{script.c_str(), false}});
  }

  insert(buildIn.begin(), buildIn.end());
}
