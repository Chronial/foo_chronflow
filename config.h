// {8426005A-98F3-42fe-A335-C0929C7DBC69}
static const GUID guid_cfgFilter = { 0x8426005a, 0x98f3, 0x42fe, { 0xa3, 0x35, 0xc0, 0x92, 0x9c, 0x7d, 0xbc, 0x69 } };
cfg_string cfgFilter(guid_cfgFilter, "");

// {34E7B6A7-50FF-4e89-8A3E-CE55E05E02EB}
static const GUID guid_cfgGroup = { 0x34e7b6a7, 0x50ff, 0x4e89, { 0x8a, 0x3e, 0xce, 0x55, 0xe0, 0x5e, 0x2, 0xeb } };
cfg_string cfgGroup(guid_cfgGroup, "%album artist%|%album%");

// {93D2D03D-A413-4e6a-BEA5-D93D19DAB0C0}
static const GUID guid_cfgSort = { 0x93d2d03d, 0xa413, 0x4e6a, { 0xbe, 0xa5, 0xd9, 0x3d, 0x19, 0xda, 0xb0, 0xc0 } };
cfg_string cfgSort(guid_cfgSort, "%album artist%|%date%|%album%");

// {36380415-1665-42b5-A2C1-CEE0F660F5FF}
static const GUID guid_cfgSortGroup = { 0x36380415, 0x1665, 0x42b5, { 0xa2, 0xc1, 0xce, 0xe0, 0xf6, 0x60, 0xf5, 0xff } };
cfg_bool cfgSortGroup(guid_cfgSortGroup, true);

// {F1A0AFF9-A8EF-4b54-B4CB-54335FBCC7C9}
static const GUID guid_cfgPlSortPl = { 0xf1a0aff9, 0xa8ef, 0x4b54, { 0xb4, 0xcb, 0x54, 0x33, 0x5f, 0xbc, 0xc7, 0xc9 } };
cfg_bool cfgPlSortPl(guid_cfgPlSortPl, false);

// {F7F9192E-09D8-477c-BEBE-DE73C085E0FE}
static const GUID guid_cfgInnerSort = { 0xf7f9192e, 0x9d8, 0x477c, { 0xbe, 0xbe, 0xde, 0x73, 0xc0, 0x85, 0xe0, 0xfe } };
cfg_string cfgInnerSort(guid_cfgInnerSort, "$num(%tracknumber%,3)");

// {81395AF7-1CC7-4dbe-A390-938F4D5FF224}
static const GUID guid_cfgSources = { 0x81395af7, 0x1cc7, 0x4dbe, { 0xa3, 0x90, 0x93, 0x8f, 0x4d, 0x5f, 0xf2, 0x24 } };
cfg_string cfgSources(guid_cfgSources, "$replace(%path%,%filename_ext%,)folder.jpg");

// {3D438A94-7B49-41af-8F39-BE76E6BF47F0}
static const GUID guid_cfgImgNoCover = { 0x3d438a94, 0x7b49, 0x41af, { 0x8f, 0x39, 0xbe, 0x76, 0xe6, 0xbf, 0x47, 0xf0 } };
cfg_string cfgImgNoCover(guid_cfgImgNoCover, ".\\components\\no-cover.png");

// {7FA5E2CC-04EB-4585-9209-D188CD824C79}
static const GUID guid_cfgImgLoading = { 0x7fa5e2cc, 0x4eb, 0x4585, { 0x92, 0x9, 0xd1, 0x88, 0xcd, 0x82, 0x4c, 0x79 } };
cfg_string cfgImgLoading(guid_cfgImgLoading, ".\\components\\loading.png");


// {C34448FB-11B2-4f6f-87BB-D0E31CE77C45}
static const GUID guid_cfgDoubleClick = { 0xc34448fb, 0x11b2, 0x4f6f, { 0x87, 0xbb, 0xd0, 0xe3, 0x1c, 0xe7, 0x7c, 0x45 } };
cfg_string cfgDoubleClick(guid_cfgDoubleClick, "");

// {1E6177E9-1C29-41dc-BCB9-047EC5FD1816}
static const GUID guid_cfgMiddleClick = { 0x1e6177e9, 0x1c29, 0x41dc, { 0xbc, 0xb9, 0x4, 0x7e, 0xc5, 0xfd, 0x18, 0x16 } };
cfg_string cfgMiddleClick(guid_cfgMiddleClick, "");

// {30AFB194-E81A-4130-91A9-1BCA1F4E3715}
static const GUID guid_cfgEnterKey = { 0x30afb194, 0xe81a, 0x4130, { 0x91, 0xa9, 0x1b, 0xca, 0x1f, 0x4e, 0x37, 0x15 } };
cfg_string cfgEnterKey(guid_cfgEnterKey, "");





// Non-config vars
// {53209101-8AAE-4256-90A4-F67417F4F75C}
static const GUID guid_sessionSelectedCover = { 0x53209101, 0x8aae, 0x4256, { 0x90, 0xa4, 0xf6, 0x74, 0x17, 0xf4, 0xf7, 0x5c } };
cfg_int sessionSelectedCover(guid_sessionSelectedCover, 0);
