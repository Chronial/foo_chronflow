// {8426005A-98F3-42fe-A335-C0929C7DBC69}
static const GUID guid_cfgFilter = { 0x8426005a, 0x98f3, 0x42fe, { 0xa3, 0x35, 0xc0, 0x92, 0x9c, 0x7d, 0xbc, 0x69 } };
cfg_string cfgFilter(guid_cfgFilter, "%album artist%|%album%");

// {81395AF7-1CC7-4dbe-A390-938F4D5FF224}
static const GUID guid_cfgSources = { 0x81395af7, 0x1cc7, 0x4dbe, { 0xa3, 0x90, 0x93, 0x8f, 0x4d, 0x5f, 0xf2, 0x24 } };
cfg_string cfgSources(guid_cfgSources, "$replace(%path%,%filename_ext%,)folder.jpg");