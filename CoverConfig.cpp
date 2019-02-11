#include "CoverConfig.h"

#include "base.h"

class BuildInCoverConfigs : public std::vector<CoverConfig> {
 public:
  BuildInCoverConfigs() {
    auto map = builtInCoverConfigs;
    for (auto& [name, script] : builtInCoverConfigs) {
      this->push_back(CoverConfig{name, script, true});
    }
  }
};
const BuildInCoverConfigs buildIn{};

cfg_coverConfigs::cfg_coverConfigs(const GUID& p_guid)
    : cfg_var(p_guid), std::vector<CoverConfig>(buildIn) {}

CoverConfig* cfg_coverConfigs::getPtrByName(const char* name) {
  for (auto& config : *this) {
    if (!stricmp_utf8(config.name, name))
      return &config;
  }
  return nullptr;
}
bool cfg_coverConfigs::removeItemByName(const char* name) {
  for (auto it = begin(); it != end(); ++it) {
    if (!stricmp_utf8(it->name, name)) {
      erase(it);
      return true;
    }
  }
  return false;
}

void cfg_coverConfigs::sortByName() {
  std::sort(
      begin(), end(), [](auto a, auto b) { return stricmp_utf8(a.name, b.name) < 0; });
}

void cfg_coverConfigs::get_data_raw(stream_writer* p_stream, abort_callback& p_abort) {
  p_stream->write_lendian_t(version, p_abort);
  int c = std::count_if(begin(), end(), [](auto x) { return !x.buildIn; });
  p_stream->write_lendian_t(c, p_abort);
  for (auto& config : *this) {
    if (config.buildIn)
      continue;
    p_stream->write_string(config.name, p_abort);
    p_stream->write_string(config.script, p_abort);
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
  reserve(c);
  for (int i = 0; i < c; i++) {
    CoverConfig config;
    p_stream->read_string(config.name, p_abort);
    p_stream->read_string(config.script, p_abort);
    config.buildIn = false;
    push_back(std::move(config));
  }

  insert(end(), buildIn.begin(), buildIn.end());
  sortByName();
}
