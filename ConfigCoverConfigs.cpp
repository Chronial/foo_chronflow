#include "ConfigCoverConfigs.h"

namespace coverflow {

CoverConfigMap builtInCoverConfigs() {
  CoverConfigMap map;
  const char** p = builtInCoverConfigArray;
  while (*p != nullptr) {
    const char* name = *(p++);
    const char* script = *(p++);
    map[name] = CoverConfig{script, true};
  }
  PFC_ASSERT(map.count(defaultCoverConfig) == 1);
  PFC_ASSERT(map.count(coverConfigTemplate) == 1);
  return map;
}

int GetCoverConfigPosition(CoverConfigMap configs, pfc::string8 name) {
    int pos = -1;
    CoverConfigMap::const_iterator it = std::find_if(configs.begin(), configs.end(),
                     [&](auto b) {
                       pfc::string8 aname(b.first.c_str());
                       return aname.equals(name);
                     });

    if (it != configs.end()) {
      //std::distance(configs.begin(), it);
      std::vector<std::pair<std::string, CoverConfig>> v1(
            (CoverConfigMap::const_iterator)configs.begin(), it);
      pos = v1.size();
    }
    return pos;
}
pfc::string8 GetCoverConfigScript(CoverConfigMap configs, pfc::string8 name) {
  auto it = configs.find(name.c_str());
  if (it != configs.end())
    return it->second.script.c_str();

  return "";
}

//int GetCoverConfigPosition_old(CoverConfigMap configs, pfc::string8 name) {
//  int counter = 0;
//  for (CoverConfigMap::const_iterator it = configs.begin(); it != configs.end(); ++it) {
//    if (name.equals(it->first.c_str()))
//      break;
//    counter++;
//  }
//  return counter;
//}

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

  if (v > version) {
    MessageBox(nullptr,
               uT(PFC_string_formatter()
                   << "Couldn't load cover configs (incompatible version)\r\nIf you have not done it "
                   << "yet, kill foobar with the taskmanager and make a backup of your "
                   << AppName
                   << " configs with the version of " << AppNameInternal <<
                   " you created them with."),
               uT(PFC_string_formatter() << AppNameInternal << " - loading Error"),
               MB_ICONERROR);
    return;
  }
  p_stream->read_lendian_t(c, p_abort);
  clear();
  for (int i = 0; i < c; i++) {
    pfc::string8 name;
    pfc::string8 script;
    p_stream->read_string(name, p_abort);
    p_stream->read_string(script, p_abort);
    if (v == 1) {
      script = script << "\n\n// Decide whether the cover title should be enabled.\n" <<
                         "// Return true to apply display panel settings\n" <<
                         "function enableCoverTitle() {\n" <<
                         "\treturn true;" << "\n" <<
                         "}";
      script = script << "\n\n// Decide whether the cover PNG8 alpha channel should be enabled.\n" <<
                         "// Return true to apply display panel settings\n" <<
                         "function enableCoverPngAlpha() {\n" <<
                         "\treturn true;" << "\n" <<
                         "}";
    }
    insert({name.c_str(), CoverConfig{script.c_str(), false}});
  }
  auto builtIn = builtInCoverConfigs();
  insert(builtIn.begin(), builtIn.end());
}
} // namespace coverflow
