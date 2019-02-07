#include "CoverConfig.h"

#include "base.h"

namespace {
class CoverConfig_compareName : public pfc::list_base_t<CoverConfig>::sort_callback {
 public:
  inline static int static_compare(const CoverConfig& a, const CoverConfig& b) {
    return stricmp_utf8(a.name, b.name);
  }
  int compare(const CoverConfig& a, const CoverConfig& b) override {
    return static_compare(a, b);
  }
};

class BuildInCoverConfigs : public pfc::list_t<CoverConfig> {
 public:
  BuildInCoverConfigs() {
    auto map = builtInCoverConfigs;
    for (auto& [name, script] : builtInCoverConfigs) {
      this->add_item(CoverConfig{name, script, true});
    }
  }
};
const BuildInCoverConfigs buildIn{};
}  // namespace

cfg_coverConfigs::cfg_coverConfigs(const GUID& p_guid) : cfg_var(p_guid) {
  remove_all();
  add_items(buildIn);
}

CoverConfig* cfg_coverConfigs::getPtrByName(const char* name) {
  int count = get_count();
  for (int i = 0; i < count; i++) {
    if (!stricmp_utf8(m_buffer[i].name, name)) {
      return &m_buffer[i];
    }
  }
  return nullptr;
}
bool cfg_coverConfigs::removeItemByName(const char* name) {
  int count = get_count();
  for (int i = 0; i < count; i++) {
    if (!stricmp_utf8(m_buffer[i].name, name)) {
      remove_by_idx(i);
      return true;
    }
  }
  return false;
}
void cfg_coverConfigs::sortByName() {
  CoverConfig_compareName callback{};
  sort(callback);
}

void cfg_coverConfigs::get_data_raw(stream_writer* p_stream, abort_callback& p_abort) {
  p_stream->write_lendian_t(version, p_abort);
  int count = get_count();
  int c = 0;
  for (int i = 0; i < count; i++) {
    if (!m_buffer[i].buildIn)
      c++;
  }
  p_stream->write_lendian_t(c, p_abort);
  for (int i = 0; i < count; i++) {
    if (m_buffer[i].buildIn)
      continue;
    p_stream->write_string(m_buffer[i].name, p_abort);
    p_stream->write_string(m_buffer[i].script, p_abort);
  }
}

void cfg_coverConfigs::set_data_raw(stream_reader* p_stream, t_size p_sizehint,
                                    abort_callback& p_abort) {
  int c, v;
  p_stream->read_lendian_t(v, p_abort);
  if (v != 1) {
    MessageBox(
        0,
        L"Couldn't load cover configs (incompatible version)\r\nIf you have not done it "
        L"yet, kill foobar with the taskmanager and make a backup of your coverflow "
        L"configs with the version of foo_chronflow you created them with.",
        L"foo_chronflow - loading Error", MB_ICONERROR);
    return;
  }
  p_stream->read_lendian_t(c, p_abort);
  m_buffer.set_size(c);
  for (int i = 0; i < c; i++) {
    p_stream->read_string(m_buffer[i].name, p_abort);
    p_stream->read_string(m_buffer[i].script, p_abort);
    m_buffer[i].buildIn = false;
  }

  add_items(buildIn);
  sort_remove_duplicates_t(
      &CoverConfig_compareName::static_compare);  // just to be sure...
}
