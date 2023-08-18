#pragma once
#include "ConfigCompiledInfo.h"
#include "ConfigCoverConfigs.h"

enum VSyncMode {
  VSYNC_SLEEP_ONLY = 1,
  VSYNC_AND_SLEEP = 2,
  VSYNC_ONLY = 3,
};

enum class PlSrcFilter:uint8_t {
  ANY_PLAYLIST = 0,
  PLAYLIST,
  ACTIVE_PLAYLIST
};

//x ui
enum DispFlags {
  DISABLE_CAROUSEL    = 1 << 0,  //todo: depri
  CUST_1              = 1 << 1,
  CUST_2              = 1 << 2,
  CUST_3              = 1 << 3,
  CUST_4              = 1 << 4,
  SET_LIB_SEL         = 1 << 5,
  FOLLOW_LIB_SEL      = 1 << 6,
  FOLLOW_PLAY_NOW     = 1 << 7,
  CUST_8              = 1 << 8,
  SRC_PLAYLIST        = 1 << 9,
  SRC_ACTPLAYLIST     = 1 << 10,
  SET_PL_SEL          = 1 << 11,
  FOLLOW_PL_SEL       = 1 << 12,
  CUST_13             = 1 << 13,
  SRC_PL_UNGROUPED    = 1 << 14,
  SRC_PL_HL           = 1 << 15
};

//todo:
enum ArtFilterFlag {  
  EXC_EMBEDDED = 1 << 0,
  EXC_FALLBACK = 1 << 1,
  ONLY_EMBEDDED = 1 << 2
};

struct DBUngroupedParams {
  pfc::string8 filter = "";
  pfc::string8 group = "%Title%";
  bool sortgroup = true;
  pfc::string8 sort = "NONE";
};

struct src_state {
  std::pair<bool, bool> wholelib;
  std::pair<bool, bool> active_pl_src;
  std::pair<bool, bool> pl_src;
  std::pair<bool, bool> grouped;
  std::pair<t_size, metadb_handle_ptr>track_first;
  std::pair<t_size, metadb_handle_ptr>track_second;
};

constexpr int MaxArtIndex = 3;

namespace coverflow {
//__declspec(selectany) extern char kmenu_label_PlayListCovers[15] = "Playlist Covers";
/******************************* Behaviour tab *******************************/
__declspec(selectany) extern bool const default_CoverFollowsPlayback = true;
__declspec(selectany) extern int default_CoverFollowDelay = 15;
__declspec(selectany) extern bool const default_FindAsYouType = true;
__declspec(selectany) extern bool const default_FindAsYouTypeCaseSens = false;
__declspec(selectany) extern char const* const default_TargetPlaylist = "CoverflowMod";
__declspec(selectany) extern char const* const default_DoubleClick =
    "Send to Default Playlist ";
__declspec(selectany) extern char const* const default_MiddleClick =
    "Send to Default Playlist ";
__declspec(selectany) extern char const* const default_EnterKey =
    "Send to Default Playlist ";
__declspec(selectany) extern unsigned long const default_CustomActionFlag = 658958;
__declspec(selectany) extern int const default_CustomActionHRate = 3;
__declspec(selectany) extern bool const default_CoverFollowsLibrarySelection = false;
__declspec(selectany) extern bool const default_CoverFollowsAnonymSelection = false;
__declspec(selectany) extern bool const default_CoverFollowsPlaylistSelection = true;
__declspec(selectany) extern bool const default_CoverHighLightPlaylistSelection = false;
/***************************** Album Source tab ******************************/
__declspec(selectany) extern char const* const default_Filter = "";
__declspec(selectany) extern char const* const default_Group = "%album artist%|%album%";
__declspec(selectany) extern char const* const default_Sort =
    "%album artist%|%date%|%album%";
__declspec(selectany) extern bool const default_SortGroup = true;
__declspec(selectany) extern char const* const default_InnerSort =
    "%discnumber%|$num(%tracknumber%,3)";
__declspec(selectany) extern char const* const default_ImgNoCover = "";
__declspec(selectany) extern char const* const default_ImgLoading = "";
/***************************** Album Source tab v.2 **************************/
__declspec(selectany) extern bool const default_SourcePlaylist = false;
__declspec(selectany) extern bool const default_SourceActivePlaylist = false;
__declspec(selectany) extern char const* const default_SourcePlaylistName = "CoverFlowMod";
__declspec(selectany) extern char const* const default_SourceActivePlaylistName = "CoverFlowMod";
__declspec(selectany) extern char const* const default_SourcePlaylistGUID = pfc::print_guid(pfc::guid_null);
__declspec(selectany) extern char const* const default_SourceActivePlaylistGUID = pfc::print_guid(pfc::guid_null);
__declspec(selectany) extern bool const default_SourcePlaylistGroup = true;
__declspec(selectany) extern char const* const default_SourcePlaylistNGTitle = "%Track% ~ %Title%";
__declspec(selectany) extern bool const default_SourceLibrarySelector = false;
__declspec(selectany) extern bool const default_SourceLibrarySelectorLock = false;
/******************************** Display tab ********************************/
__declspec(selectany) extern bool const default_ShowAlbumTitle = true;
__declspec(selectany) extern char const* const default_AlbumTitle = "%album artist% - %album%";
__declspec(selectany) extern double const default_TitlePosH = 0.5;
__declspec(selectany) extern double const default_TitlePosV = 0.95;
__declspec(selectany) extern bool const default_TitleColorCustom = false;
__declspec(selectany) extern unsigned long const default_TitleColor = RGB(0, 0, 0);
__declspec(selectany) extern bool const default_TitleFontCustom = true;
__declspec(selectany) extern bool const default_PanelBgCustom = false;
__declspec(selectany) extern unsigned long const default_PanelBg = RGB(255, 255, 255);
__declspec(selectany) extern int const default_HighlightWidth = 0;
/******************************** Display tab v.2 ****************************/
__declspec(selectany) extern int const default_CoverArt = 0;
__declspec(selectany) extern int const default_CustomCoverFrontArt = 0;
__declspec(selectany) extern bool const default_CoverArtEnablePngAlpha = false;
__declspec(selectany) extern bool const default_CoverUseLegacyExternalViewer = false;
/******************************** Display tab v.2 ****************************/
__declspec(selectany) extern int const default_DisplayFlag = 0;
__declspec(selectany) extern int const default_DisplayArtFilterFlag = 0;
__declspec(selectany) extern char const* const default_DisplayExtViewerPath = "ImageGlass.exe";
/***************************** Cover Display tab *****************************/
__declspec(selectany) extern char const* const default_CoverConfigSel = defaultCoverConfig;
/****************************** Performance tab ******************************/
__declspec(selectany) extern bool const default_Multisampling = true;
__declspec(selectany) extern int const default_MultisamplingPasses = 4;
__declspec(selectany) extern int const default_TextureCacheSize = 150;
__declspec(selectany) extern int const default_MaxTextureSize = 512;
__declspec(selectany) extern bool const default_TextureCompression = false;
__declspec(selectany) extern bool const default_EmptyCacheOnMinimize = true;
__declspec(selectany) extern bool const default_VSyncMode = VSYNC_SLEEP_ONLY;
__declspec(selectany) extern bool const default_ShowFps = false;
/*********************************** Ctx Menu ********************************/
__declspec(selectany) extern bool const default_CtxShowPlaylistMenu = true;
__declspec(selectany) extern bool const default_CtxShowDisplayMenu = true;
__declspec(selectany) extern bool const default_CtxShowSelectorMenu = false;
__declspec(selectany) extern bool const default_CtxShowExtViewerMenu = true;
__declspec(selectany) extern bool const default_CtxShowActionsMenu = true;
/*********************************** Session *********************************/
__declspec(selectany) extern char const* const default_sessionSelectedCover = "";
__declspec(selectany) extern int const default_sessionSelectedConfigTab = 0;

static inline LOGFONT def_cfgTitleFont() {
  LOGFONT out{};
  wcscpy_s(out.lfFaceName, L"Verdana");
  out.lfHeight = -12;
  out.lfWeight = 400;
  return out;
}

class ConfigData : public cfg_var_legacy::cfg_var {
 public:
  ConfigData();
  virtual ~ConfigData() = default;
  void Reset();

  pfc::string8 PrefsVersion;
  /******************************* Behaviour tab *******************************/
  bool CoverFollowsPlayback;
  int CoverFollowDelay;
  bool FindAsYouType;
  bool FindAsYouTypeCaseSens;
  pfc::string8 TargetPlaylist;
  pfc::string8 DoubleClick;
  pfc::string8 MiddleClick;
  pfc::string8 EnterKey;
  int CustomActionFlag;
  int CustomActionHRate;
  bool CoverFollowsLibrarySelection;
  bool CoverFollowsAnonymSelection;
  bool CoverFollowsPlaylistSelection;
  bool CoverHighLightPlaylistSelection;
  /***************************** Album Source tab ******************************/
  pfc::string8 Filter;
  pfc::string8 Group;
  pfc::string8 Sort;
  bool SortGroup;
  pfc::string8 InnerSort;
  pfc::string8 ImgNoCover;
  pfc::string8 ImgLoading;
  /***************************** Album Source tab v.2 **************************/
  bool SourcePlaylist;
  bool SourceActivePlaylist;
  pfc::string8 SourceActivePlaylistName;
  pfc::string8 SourcePlaylistName;
  pfc::string8 SourceActivePlaylistGUID;
  pfc::string8 SourcePlaylistGUID;
  bool SourcePlaylistGroup;
  pfc::string8 SourcePlaylistNGTitle;
  bool SourceLibrarySelector;
  bool SourceLibrarySelectorLock;
  /******************************** Display tab ********************************/
  bool ShowAlbumTitle;
  pfc::string8 AlbumTitle;
  double TitlePosH;
  double TitlePosV;
  bool TitleColorCustom;
  int TitleColor;
  bool TitleFontCustom;
  LOGFONT TitleFont;
  bool PanelBgCustom;
  int PanelBg;
  int HighlightWidth;
  /******************************** Display tab v.2 ****************************/
  int CustomCoverFrontArt;
  bool CoverArtEnablePngAlpha;
  bool CoverUseLegacyExternalViewer;
  /******************************** Display tab v.4 ****************************/
  int DisplayFlag;
  int DisplayArtFilterFlag;
  pfc::string8 DisplayExtViewerPath;
  /***************************** Cover Display tab *****************************/
  cfg_coverConfigs CoverConfigs;  // no default;
  pfc::string8 CoverConfigSel;
  /****************************** Performance tab ******************************/
  bool Multisampling;
  int MultisamplingPasses;
  int TextureCacheSize;
  int MaxTextureSize;
  bool TextureCompression;
  bool EmptyCacheOnMinimize;
  int VSyncMode;
  bool ShowFps;
  /****************************** Context Menu tab *****************************/
  bool CtxShowPlaylistMenu;
  bool CtxShowDisplayMenu;
  bool CtxShowSelectorMenu;
  bool CtxShowExtViewerMenu;
  bool CtxShowActionsMenu;
  /*********************************** Session *********************************/
  pfc::string8 sessionSelectedCover;
  cfg_compiledCPInfoPtr sessionCompiledCPInfo;  // no default;
  int sessionSelectedConfigTab;

 public:


   size_t GetDisplayFlag() const {
    size_t retval = 0;

    retval |= /*coverSetsLibrarySel*/true ? DispFlags::SET_LIB_SEL : 0;
    retval |= CoverFollowsLibrarySelection ? DispFlags::FOLLOW_LIB_SEL : 0;
    retval |= /*coverSetsPlaylistSel*/true ? DispFlags::SET_PL_SEL : 0;
    retval |= CoverFollowsPlaylistSelection ? DispFlags::FOLLOW_PL_SEL : 0;

    retval |= CoverFollowsPlayback ? DispFlags::FOLLOW_PLAY_NOW : 0;

    retval |= SourcePlaylist ? DispFlags::SRC_PLAYLIST : 0;
    retval |= SourceActivePlaylist ? DispFlags::SRC_ACTPLAYLIST : 0;

    retval |= !true/*coverEnablePlaylistCoversGrouped*/ ? DispFlags::SRC_PL_UNGROUPED : 0;

    retval |= CoverHighLightPlaylistSelection ? DispFlags::SRC_PL_HL : 0;

    return retval;
  }

  int GetCCPosition(pfc::string8 coverconfigsel) {
    CoverConfigMap configs = CoverConfigs;
    return GetCoverConfigPosition(configs, coverconfigsel);
  }

  GUID GetGuidArt(int iart) const {
    switch (iart) {
      case 0:
        return album_art_ids::cover_front;
      case 1:
        return album_art_ids::cover_back;
      case 2:
        return album_art_ids::disc;
      case 3:
        return album_art_ids::artist;
      default:
        return album_art_ids::cover_front;
    }
  }

 private:
  virtual void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) override;
  virtual void set_data_raw(stream_reader* p_stream, t_size p_sizehint,
                            abort_callback& p_abort) override;
  void SetData(ConfigData& cfg, stream_reader* p_stream, abort_callback& p_abort,
               const char* version);
};
#ifndef CONFIG_DATA_H
#define CONFIG_DATA_H
__declspec(selectany) shared_ptr<ConfigData> configData = std::make_shared<ConfigData>();
#endif
}  // namespace coverflow
