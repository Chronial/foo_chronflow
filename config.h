#pragma once
#include "CoverConfig.h"
#include "cover_positions.h"

/***************************** Album Source tab ******************************/
extern cfg_string cfgFilter;
extern cfg_string cfgGroup;
extern cfg_string cfgSort;
extern cfg_bool cfgSortGroup;
extern cfg_string cfgInnerSort;
extern cfg_string cfgImgNoCover;
extern cfg_string cfgImgLoading;

/******************************* Behaviour tab *******************************/
extern cfg_bool cfgCoverFollowsPlayback;
extern cfg_int cfgCoverFollowDelay;
extern cfg_bool cfgFindAsYouType;
extern cfg_string cfgTargetPlaylist;

extern cfg_string cfgDoubleClick;
extern cfg_string cfgMiddleClick;
extern cfg_string cfgEnterKey;

/******************************** Display tab ********************************/
extern cfg_bool cfgShowAlbumTitle;
extern cfg_string cfgAlbumTitle;
extern cfg_struct_t<double> cfgTitlePosH;
extern cfg_struct_t<double> cfgTitlePosV;
extern cfg_bool cfgTitleColorCustom;
extern cfg_int_t<COLORREF> cfgTitleColor;
extern cfg_bool cfgTitleFontCustom;
extern cfg_struct_t<LOGFONT> cfgTitleFont;
extern cfg_bool cfgPanelBgCustom;
extern cfg_int cfgPanelBg;
extern cfg_int cfgHighlightWidth;

/***************************** Cover Display tab *****************************/
extern cfg_coverConfigs cfgCoverConfigs;
extern cfg_string cfgCoverConfigSel;

/****************************** Performance tab ******************************/
extern cfg_bool cfgMultisampling;
extern cfg_int cfgMultisamplingPasses;

extern cfg_int cfgTextureCacheSize;
extern cfg_int cfgMaxTextureSize;
extern cfg_bool cfgTextureCompression;
extern cfg_bool cfgEmptyCacheOnMinimize;

extern cfg_int cfgVSyncMode;
extern cfg_bool cfgShowFps;

/****************************** non-config vars ******************************/
extern cfg_string_mt sessionSelectedCover;
extern cfg_int sessionSelectedConfigTab;
extern cfg_compiledCPInfoPtr sessionCompiledCPInfo;

/*********************************** Types ***********************************/
enum VSyncMode {
  VSYNC_SLEEP_ONLY = 1,
  VSYNC_AND_SLEEP = 2,
  VSYNC_ONLY = 3,
};
