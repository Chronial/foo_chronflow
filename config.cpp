#include "config.h"

#include "CoverConfig.h"
#include "cover_positions.h"

/***************************** Album Source tab ******************************/
// {8426005A-98F3-42fe-A335-C0929C7DBC69}
static const GUID guid_cfgFilter = {
    0x8426005a, 0x98f3, 0x42fe, {0xa3, 0x35, 0xc0, 0x92, 0x9c, 0x7d, 0xbc, 0x69}};
cfg_string cfgFilter(guid_cfgFilter, "");

// {34E7B6A7-50FF-4e89-8A3E-CE55E05E02EB}
static const GUID guid_cfgGroup = {
    0x34e7b6a7, 0x50ff, 0x4e89, {0x8a, 0x3e, 0xce, 0x55, 0xe0, 0x5e, 0x2, 0xeb}};
cfg_string cfgGroup(guid_cfgGroup, "%album artist%|%album%");

// {93D2D03D-A413-4e6a-BEA5-D93D19DAB0C0}
static const GUID guid_cfgSort = {
    0x93d2d03d, 0xa413, 0x4e6a, {0xbe, 0xa5, 0xd9, 0x3d, 0x19, 0xda, 0xb0, 0xc0}};
cfg_string cfgSort(guid_cfgSort, "%album artist%|%date%|%album%");

// {36380415-1665-42b5-A2C1-CEE0F660F5FF}
static const GUID guid_cfgSortGroup = {
    0x36380415, 0x1665, 0x42b5, {0xa2, 0xc1, 0xce, 0xe0, 0xf6, 0x60, 0xf5, 0xff}};
cfg_bool cfgSortGroup(guid_cfgSortGroup, true);

// {F7F9192E-09D8-477c-BEBE-DE73C085E0FE}
static const GUID guid_cfgInnerSort = {
    0xf7f9192e, 0x9d8, 0x477c, {0xbe, 0xbe, 0xde, 0x73, 0xc0, 0x85, 0xe0, 0xfe}};
cfg_string cfgInnerSort(guid_cfgInnerSort, "%discnumber%|$num(%tracknumber%,3)");

// {3D438A94-7B49-41af-8F39-BE76E6BF47F0}
static const GUID guid_cfgImgNoCover = {
    0x3d438a94, 0x7b49, 0x41af, {0x8f, 0x39, 0xbe, 0x76, 0xe6, 0xbf, 0x47, 0xf0}};
cfg_string cfgImgNoCover(guid_cfgImgNoCover, "");

// {7FA5E2CC-04EB-4585-9209-D188CD824C79}
static const GUID guid_cfgImgLoading = {
    0x7fa5e2cc, 0x4eb, 0x4585, {0x92, 0x9, 0xd1, 0x88, 0xcd, 0x82, 0x4c, 0x79}};
cfg_string cfgImgLoading(guid_cfgImgLoading, "");

/******************************* Behaviour tab *******************************/
// {5B915FEB-5FED-4edc-98D7-26F5BC991B3D}
static const GUID guid_cfgCoverFollowsPlayback = {
    0x5b915feb, 0x5fed, 0x4edc, {0x98, 0xd7, 0x26, 0xf5, 0xbc, 0x99, 0x1b, 0x3d}};
cfg_bool cfgCoverFollowsPlayback(guid_cfgCoverFollowsPlayback, true);

// {7AA69C60-1528-4ce3-86ED-D413A8A93CE1}
static const GUID guid_cfgCoverFollowDelay = {
    0x7aa69c60, 0x1528, 0x4ce3, {0x86, 0xed, 0xd4, 0x13, 0xa8, 0xa9, 0x3c, 0xe1}};
cfg_int cfgCoverFollowDelay(guid_cfgCoverFollowDelay, 15);

// {53BF0A95-12E2-440a-B2FB-F75BE90764E2}
static const GUID guid_cfgFindAsYouType = {
    0x53bf0a95, 0x12e2, 0x440a, {0xb2, 0xfb, 0xf7, 0x5b, 0xe9, 0x7, 0x64, 0xe2}};
cfg_bool cfgFindAsYouType(guid_cfgFindAsYouType, true);

// {A5763153-444A-4649-82E4-E5E3D395DD42}
static const GUID guid_cfgTargetPlaylist = {
    0xa5763153, 0x444a, 0x4649, {0x82, 0xe4, 0xe5, 0xe3, 0xd3, 0x95, 0xdd, 0x42}};
cfg_string cfgTargetPlaylist(guid_cfgTargetPlaylist, "#Chronflow");

// {C34448FB-11B2-4f6f-87BB-D0E31CE77C45}
static const GUID guid_cfgDoubleClick = {
    0xc34448fb, 0x11b2, 0x4f6f, {0x87, 0xbb, 0xd0, 0xe3, 0x1c, 0xe7, 0x7c, 0x45}};
cfg_string cfgDoubleClick(guid_cfgDoubleClick, "Replace Default Playlist ");

// {1E6177E9-1C29-41dc-BCB9-047EC5FD1816}
static const GUID guid_cfgMiddleClick = {
    0x1e6177e9, 0x1c29, 0x41dc, {0xbc, 0xb9, 0x4, 0x7e, 0xc5, 0xfd, 0x18, 0x16}};
cfg_string cfgMiddleClick(guid_cfgMiddleClick, "Add to new Playlist ");

// {30AFB194-E81A-4130-91A9-1BCA1F4E3715}
static const GUID guid_cfgEnterKey = {
    0x30afb194, 0xe81a, 0x4130, {0x91, 0xa9, 0x1b, 0xca, 0x1f, 0x4e, 0x37, 0x15}};
cfg_string cfgEnterKey(guid_cfgEnterKey, "Replace Default Playlist ");

/******************************** Display tab ********************************/
// {3CA53569-36E3-4f55-98E9-5FDD27C6D041}
static const GUID guid_cfgShowAlbumTitle = {
    0x3ca53569, 0x36e3, 0x4f55, {0x98, 0xe9, 0x5f, 0xdd, 0x27, 0xc6, 0xd0, 0x41}};
cfg_bool cfgShowAlbumTitle(guid_cfgShowAlbumTitle, true);

// {85C3038A-EC62-416f-9D76-2B86D0F7493C}
static const GUID guid_cfgAlbumTitle = {
    0x85c3038a, 0xec62, 0x416f, {0x9d, 0x76, 0x2b, 0x86, 0xd0, 0xf7, 0x49, 0x3c}};
cfg_string cfgAlbumTitle(guid_cfgAlbumTitle, "%album artist% - %album%");
service_ptr_t<titleformat_object> cfgAlbumTitleScript;

// {6B35F73F-C754-40ab-A8EB-3BA86CB43AF5}
static const GUID guid_cfgTitlePosH = {
    0x6b35f73f, 0xc754, 0x40ab, {0xa8, 0xeb, 0x3b, 0xa8, 0x6c, 0xb4, 0x3a, 0xf5}};
cfg_struct_t<double> cfgTitlePosH(guid_cfgTitlePosH, 0.5);

// {8441C311-7EF1-47d0-BCBF-D7B6CEF5392C}
static const GUID guid_cfgTitlePosV = {
    0x8441c311, 0x7ef1, 0x47d0, {0xbc, 0xbf, 0xd7, 0xb6, 0xce, 0xf5, 0x39, 0x2c}};
cfg_struct_t<double> cfgTitlePosV(guid_cfgTitlePosV, 0.92);

// {10820ADB-EAC6-403f-89B3-A33E2F800DD7}
static const GUID guid_cfgTitleColor = {
    0x10820adb, 0xeac6, 0x403f, {0x89, 0xb3, 0xa3, 0x3e, 0x2f, 0x80, 0xd, 0xd7}};
cfg_int_t<COLORREF> cfgTitleColor(guid_cfgTitleColor, RGB(0, 0, 0));

// {3BBA50BD-207E-43b2-95F1-44B67B898C26}
static const GUID guid_cfgTitleFont = {
    0x3bba50bd, 0x207e, 0x43b2, {0x95, 0xf1, 0x44, 0xb6, 0x7b, 0x89, 0x8c, 0x26}};
static inline LOGFONT def_cfgTitleFont() {
  LOGFONT out{};
  wcscpy_s(out.lfFaceName, L"Verdana");
  out.lfHeight = -18;
  out.lfWeight = 400;
  return out;
}
cfg_struct_t<LOGFONT> cfgTitleFont(guid_cfgTitleFont, def_cfgTitleFont());

// {8AC48B7A-E38E-40fb-B6BE-1A0CD0D81D30}
static const GUID guid_cfgPanelBg = {
    0x8ac48b7a, 0xe38e, 0x40fb, {0xb6, 0xbe, 0x1a, 0xc, 0xd0, 0xd8, 0x1d, 0x30}};
cfg_int cfgPanelBg(guid_cfgPanelBg, RGB(255, 255, 255));

// {495F43F9-C6A9-4289-AA1A-C871B50C5F62}
static const GUID guid_cfgHighlightWidth = {
    0x495f43f9, 0xc6a9, 0x4289, {0xaa, 0x1a, 0xc8, 0x71, 0xb5, 0xc, 0x5f, 0x62}};
cfg_int cfgHighlightWidth(guid_cfgHighlightWidth, 0);

/***************************** Cover Display tab *****************************/
// {7C3CFFF9-A881-476e-ACE7-503512F75C14}
static const GUID guid_cfgCoverConfigs = {
    0x7c3cfff9, 0xa881, 0x476e, {0xac, 0xe7, 0x50, 0x35, 0x12, 0xf7, 0x5c, 0x14}};
cfg_coverConfigs cfgCoverConfigs(guid_cfgCoverConfigs);

// {A6F566E7-9800-4de8-BF72-22FAA4ADBB6B}
static const GUID guid_cfgCoverConfigSel = {
    0xa6f566e7, 0x9800, 0x4de8, {0xbf, 0x72, 0x22, 0xfa, 0xa4, 0xad, 0xbb, 0x6b}};
cfg_string cfgCoverConfigSel(guid_cfgCoverConfigSel, "Default (build-in)");

/****************************** Performance tab ******************************/
// {11053387-12C8-4f16-8BD0-7C6FF1523252}
static const GUID guid_cfgMultisampling = {
    0x11053387, 0x12c8, 0x4f16, {0x8b, 0xd0, 0x7c, 0x6f, 0xf1, 0x52, 0x32, 0x52}};
cfg_bool cfgMultisampling(guid_cfgMultisampling, true);

// {B5280C57-CC06-4ee9-A8A3-4F05E6925A62}
static const GUID guid_cfgMultisamplingPasses = {
    0xb5280c57, 0xcc06, 0x4ee9, {0xa8, 0xa3, 0x4f, 0x5, 0xe6, 0x92, 0x5a, 0x62}};
cfg_int cfgMultisamplingPasses(guid_cfgMultisamplingPasses, 4);

// {036F84C3-B3F7-4aee-873C-DC0EFC46B0D3}
static const GUID guid_cfgTextureCacheSize = {
    0x36f84c3, 0xb3f7, 0x4aee, {0x87, 0x3c, 0xdc, 0xe, 0xfc, 0x46, 0xb0, 0xd3}};
cfg_int cfgTextureCacheSize(guid_cfgTextureCacheSize, 150);

// {87491567-6F1C-4339-BF05-FAFE9AF73820}
static const GUID guid_cfgMaxTextureSize = {
    0x87491567, 0x6f1c, 0x4339, {0xbf, 0x5, 0xfa, 0xfe, 0x9a, 0xf7, 0x38, 0x20}};
cfg_int cfgMaxTextureSize(guid_cfgMaxTextureSize, 512);

// {B3177E9B-7188-4cd1-91AD-0896E5D23292}
static const GUID guid_cfgTextureCompression = {
    0xb3177e9b, 0x7188, 0x4cd1, {0x91, 0xad, 0x8, 0x96, 0xe5, 0xd2, 0x32, 0x92}};
cfg_bool cfgTextureCompression(guid_cfgTextureCompression, false);

// {427CE6B2-DF59-4253-BBC0-157C7A91F226}
static const GUID guid_cfgEmptyCacheOnMinimize = {
    0x427ce6b2, 0xdf59, 0x4253, {0xbb, 0xc0, 0x15, 0x7c, 0x7a, 0x91, 0xf2, 0x26}};
cfg_bool cfgEmptyCacheOnMinimize(guid_cfgEmptyCacheOnMinimize, true);

// {7EFEB474-EEDC-4242-AFA7-6A8AA4C08FF9}
static const GUID guid_cfgVSyncMode = {
    0x7efeb474, 0xeedc, 0x4242, {0xaf, 0xa7, 0x6a, 0x8a, 0xa4, 0xc0, 0x8f, 0xf9}};
cfg_int cfgVSyncMode(guid_cfgVSyncMode, VSYNC_SLEEP_ONLY);

// {34AEE526-264B-4c82-B336-81D398536871}
static const GUID guid_cfgShowFps = {
    0x34aee526, 0x264b, 0x4c82, {0xb3, 0x36, 0x81, 0xd3, 0x98, 0x53, 0x68, 0x71}};
cfg_bool cfgShowFps(guid_cfgShowFps, false);

// Non-config vars
// {53209101-8AAE-4256-90A4-F67417F4F75C}
static const GUID guid_sessionSelectedCover = {
    0x53209101, 0x8aae, 0x4256, {0x90, 0xa4, 0xf6, 0x74, 0x17, 0xf4, 0xf7, 0x5c}};
cfg_int sessionSelectedCover(guid_sessionSelectedCover, 0);

// {6DDC4C81-7525-46d0-B9B8-0E8DE36957A8}
static const GUID guid_sessionCompiledCPInfo = {
    0x6ddc4c81, 0x7525, 0x46d0, {0xb9, 0xb8, 0xe, 0x8d, 0xe3, 0x69, 0x57, 0xa8}};
cfg_compiledCPInfoPtr sessionCompiledCPInfo(guid_sessionCompiledCPInfo);

// {85744FC7-38A3-47e7-8080-3744431211E6}
static const GUID guid_sessionSelectedConfigTab = {
    0x85744fc7, 0x38a3, 0x47e7, {0x80, 0x80, 0x37, 0x44, 0x43, 0x12, 0x11, 0xe6}};
cfg_int sessionSelectedConfigTab(guid_sessionSelectedConfigTab, 0);
