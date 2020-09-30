#pragma once
#include "ConfigCompiledInfo.h"
#include "ConfigCoverConfigs.h"

enum VSyncMode {
  VSYNC_SLEEP_ONLY = 1,
  VSYNC_AND_SLEEP = 2,
  VSYNC_ONLY = 3,
};

struct DBPlaylistModeParams {
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


int const MaxArtIndex = 3;
int const _PREFS_VERSION = 2;

namespace coverflow {
/******************************* Behaviour tab *******************************/
__declspec(selectany) extern bool const default_CoverFollowsPlayback = false;
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
__declspec(selectany) extern bool const default_SourcePlaylistGroup = true;
__declspec(selectany) extern char const* const default_SourcePlaylistNGTitle = "%Track% ~ %Title%";
__declspec(selectany) extern bool const default_SourceLibrarySelector = false;
__declspec(selectany) extern bool const default_SourceLibrarySelectorLock = false;
/******************************** Display tab ********************************/
__declspec(selectany) extern bool const default_ShowAlbumTitle = true;
__declspec(selectany) extern char const* const default_AlbumTitle = "%album artist%$crlf()%album%";
__declspec(selectany) extern double const default_TitlePosH = 0.5;
__declspec(selectany) extern double const default_TitlePosV = 0.92;
__declspec(selectany) extern bool const default_TitleColorCustom = false;
__declspec(selectany) extern unsigned long const default_TitleColor = RGB(0, 0, 0);
__declspec(selectany) extern bool const default_TitleFontCustom = true;
__declspec(selectany) extern bool const default_PanelBgCustom = false;
__declspec(selectany) extern unsigned long const default_PanelBg = RGB(255, 255, 255);
__declspec(selectany) extern int const default_HighlightWidth = 0;
/******************************** Display tab v.2 ****************************/
__declspec(selectany) extern int const default_CenterArt = 0;
__declspec(selectany) extern int const default_CustomCoverFrontArt = 0;
__declspec(selectany) extern bool const default_CoverArtEnablePngAlpha = false;
__declspec(selectany) extern bool const default_CoverUseLegacyExternalViewer = false;
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
__declspec(selectany) extern bool const default_CtxHidePlaylistMenu = false;
__declspec(selectany) extern bool const default_CtxHideDisplayMenu = true;
__declspec(selectany) extern bool const default_CtxHideSelectorMenu = true;
__declspec(selectany) extern bool const default_CtxHideExtViewerMenu = true;
__declspec(selectany) extern bool const default_CtxHideActionsMenu = false;
/*********************************** Session *********************************/
__declspec(selectany) extern char const* const default_sessionSelectedCover = "";
__declspec(selectany) extern int const default_sessionSelectedConfigTab = 0;

static inline LOGFONT def_cfgTitleFont() {
  LOGFONT out{};
  wcscpy_s(out.lfFaceName, L"Verdana");
  out.lfHeight = -18;
  out.lfWeight = 400;
  return out;
}

class ConfigData : public cfg_var {
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
  t_uint32 CustomActionFlag;
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
  t_uint32 TitleColor;
  bool TitleFontCustom;
  LOGFONT TitleFont;
  bool PanelBgCustom;
  t_uint32 PanelBg;
  int HighlightWidth;
  /******************************** Display tab v.2 ****************************/
  int CenterArt;
  int CustomCoverFrontArt;
  bool CoverArtEnablePngAlpha;
  bool CoverUseLegacyExternalViewer;
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
  bool CtxHidePlaylistMenu;
  bool CtxHideDisplayMenu;
  bool CtxHideSelectorMenu;
  bool CtxHideExtViewerMenu;
  bool CtxHideActionsMenu;
  /*********************************** Session *********************************/
  pfc::string8 sessionSelectedCover;
  cfg_compiledCPInfoPtr sessionCompiledCPInfo;  // no default;
  int sessionSelectedConfigTab;

 public:
  void GetStateTrackInfo(t_size &trackpos, metadb_handle_ptr & track) {
    //this may also be run from on_playlist_activate callback
    //from that callback function gets focus and track

    const t_size plsource = FindSourcePlaylist(0);
    static_api_ptr_t<playlist_manager> pm;
    if (playlist_manager::get()->playlist_get_item_count(plsource)) {
        //selected
        bit_array_bittable selmask;
        t_size trackpos = selmask.find_first(true, 0, selmask.size());
        if (selmask.size() > 0 && trackpos != pfc_infinite) {
          metadb_handle_list msl;
          track = pm->playlist_get_item_handle(plsource, trackpos);
          //trackpos = configData->CoverFollowsPlaylistSelection? trackpos : 0;
        }
        else {
          //playlist_manager::get()->playlist_find_item(plsource, track, trackpos);
          //focused
          if (pm->playlist_get_focus_item_handle(track, plsource)) {
            trackpos = pm->playlist_get_focus_item(plsource);
            pm->playlist_find_item_selected(plsource, track, trackpos);
            //trackpos = configData->CoverFollowsPlaylistSelection ? trackpos : 0;
          } else {
            track = pm->playlist_get_item_handle(plsource, 0);
            trackpos = 0;
          }
        }
    } else
      trackpos = pfc_infinite;
  }
  void GetState(src_state& srcstate, bool init = true) {
    if (init) {
      srcstate.wholelib.first = !(SourcePlaylist | SourceActivePlaylist);
      srcstate.active_pl_src.first = SourceActivePlaylist;
      srcstate.pl_src.first = SourcePlaylist;
      srcstate.grouped.first = SourcePlaylistGroup;
      if (!srcstate.wholelib.first) {
        GetStateTrackInfo(srcstate.track_first.first, srcstate.track_first.second);
      }
    } else {
      srcstate.wholelib.second = !(SourcePlaylist | SourceActivePlaylist);
      srcstate.active_pl_src.second = SourceActivePlaylist;
      srcstate.pl_src.second = SourcePlaylist;
      srcstate.grouped.second = SourcePlaylistGroup;
      if (!srcstate.wholelib.second) {
        GetStateTrackInfo(srcstate.track_second.first, srcstate.track_second.second);
      }
    }
  }

  //bool StateIsEmpty(src_state srcstate) {
  //  if (srcstate.wholelib.first == false &&
  //      !(srcstate.pl_src.first || srcstate.active_pl_src.first))
  //    return true;
  //  else
  //    return false;
  //}

  int GetCCPosition() {
    CoverConfigMap configs = CoverConfigs;
    pfc::string8 name = CoverConfigSel;
    return GetCoverConfigPosition(configs, name);
  }

  bool IsWholeLibrary() const { return (!SourcePlaylist && !SourceActivePlaylist); }
  bool IsPlaylistSourceModeUngrouped() const {
    return (!IsWholeLibrary() && !SourcePlaylistGroup);
  }
  bool IsSourceOnAndMirrored(bool fromActive) const {
    if (fromActive)
    return (SourceActivePlaylist &&
            (stricmp_utf8(SourceActivePlaylistName, SourcePlaylistName) == 0));
    else
      return (SourcePlaylist &&
              (stricmp_utf8(SourceActivePlaylistName, SourcePlaylistName) == 0));
  }

  bool IsSourcePlaylistOn(t_size playlist, int mode) {
    const t_size srcplaylist = FindSourcePlaylist(mode);
    return (srcplaylist != ~0 && srcplaylist == playlist);
  }

  bool IsSourcePlaylistOn(pfc::string8 playlist, int mode) {
    if (IsWholeLibrary()) {
      return false;
    }
    switch (mode) {
    case 0:
      if (!IsWholeLibrary()) {
        if (SourceActivePlaylist)
          return (stricmp_utf8(playlist, SourceActivePlaylistName) == 0);
        else if (SourcePlaylist)
          return (stricmp_utf8(playlist, SourcePlaylistName) == 0);
      }
      break;
    case 1:
      if (SourcePlaylist)
        return (stricmp_utf8(playlist, SourcePlaylistName) == 0);
      else
        return false;
      break;
    case 2:
      if (SourceActivePlaylist)
        return (stricmp_utf8(playlist, SourceActivePlaylistName) == 0);
      else
        return false;
      break;
    }
  }
  pfc::string8 InSourePlaylistGetName() {
    if (!IsWholeLibrary()) {
      return SourceActivePlaylist ? SourceActivePlaylistName : SourcePlaylistName;
    } else {
      return {};
    }
  }
  t_size FindSourcePlaylist(t_size mode) const {
    if (IsWholeLibrary()) {
      return pfc_infinite;
    }
    switch (mode) {
    case 0:
      if (SourceActivePlaylist) {
        return playlist_manager::get()->find_playlist(SourceActivePlaylistName);
      } else
        return playlist_manager::get()->find_playlist(SourcePlaylistName);
      break;
    case 1:
      if (SourcePlaylist)
        return playlist_manager::get()->find_playlist(SourcePlaylistName);
      break;
    case 2:
      if (SourceActivePlaylist)
        return playlist_manager::get()->find_playlist(SourceActivePlaylistName);
      break;
    }
    return pfc_infinite;
  }
  int SequenceCenterArt(int val, bool prev, bool ctrl) {
    int iret = -1;
    if (prev) {
      if (ctrl)
        iret = 0;
      else
        iret = (val == 0 ? MaxArtIndex : val - 1);
    } else {
      if (ctrl)
        iret = MaxArtIndex;
      else
        iret = (val >= MaxArtIndex ? 0 : val + 1);
    }
    return iret;
  }
  GUID GetGuiArt(int iart) const {
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
