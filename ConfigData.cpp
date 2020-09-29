#include "stdafx.h"
#include "ConfigGuids.h"
#include "ConfigData.h"

#include "utils.h"

namespace coverflow {
ConfigData::ConfigData()
  : cfg_var(ConfigDataId),
    PrefsVersion(component_PREF_VER),
    /******************************* Behaviour tab *******************************/
    CoverFollowsPlayback(default_CoverFollowsPlayback),
    CoverFollowDelay(default_CoverFollowDelay),
    FindAsYouType(default_FindAsYouType),
    FindAsYouTypeCaseSens(default_FindAsYouTypeCaseSens),
    TargetPlaylist(default_TargetPlaylist),
    DoubleClick(default_DoubleClick),
    MiddleClick(default_MiddleClick),
    EnterKey(default_EnterKey),
    CustomActionFlag(default_CustomActionFlag),
    CustomActionHRate(default_CustomActionHRate),
    CoverFollowsLibrarySelection(default_CoverFollowsLibrarySelection),
    CoverFollowsAnonymSelection(default_CoverFollowsAnonymSelection),
    CoverFollowsPlaylistSelection(default_CoverFollowsPlaylistSelection),
    CoverHighLightPlaylistSelection(default_CoverHighLightPlaylistSelection),

    /***************************** Album Source tab ******************************/
    Filter(default_Filter),
    Group(default_Group),
    Sort(default_Sort),
    SortGroup(default_SortGroup),
    InnerSort(default_InnerSort),
    ImgNoCover(default_ImgNoCover),
    ImgLoading(default_ImgLoading),
    /***************************** Album Source tab v2 ***************************/
    SourcePlaylist(default_SourcePlaylist),
    SourceActivePlaylist(default_SourceActivePlaylist),
    SourceActivePlaylistName(default_SourceActivePlaylistName),
    SourcePlaylistName(default_SourcePlaylistName),
    SourcePlaylistGroup(default_SourcePlaylistGroup),
    SourcePlaylistNGTitle(default_SourcePlaylistNGTitle),
    SourceLibrarySelector(default_SourceLibrarySelector),
    SourceLibrarySelectorLock(default_SourceLibrarySelectorLock),
    /******************************** Display tab ********************************/
    ShowAlbumTitle(default_ShowAlbumTitle),
    AlbumTitle(default_AlbumTitle),
    TitlePosH(default_TitlePosH),
    TitlePosV(default_TitlePosV),
    TitleColorCustom(default_TitleColorCustom),
    TitleColor(default_TitleColor),
    TitleFontCustom(default_TitleFontCustom),
    TitleFont(def_cfgTitleFont()),
    PanelBgCustom(default_PanelBgCustom),
    PanelBg(default_PanelBg),
    HighlightWidth(default_HighlightWidth),
    /******************************** Display tab v.2 ****************************/
    CenterArt(default_CenterArt),
    CustomCoverFrontArt(default_CustomCoverFrontArt),
    CoverArtEnablePngAlpha(default_CoverArtEnablePngAlpha),
    CoverUseLegacyExternalViewer(default_CoverUseLegacyExternalViewer),
    /***************************** Cover Display tab *****************************/
    CoverConfigs(guid_ConfigDataCustomConfigs),
    CoverConfigSel(default_CoverConfigSel),
    /****************************** Performance tab ******************************/
    Multisampling(default_Multisampling),
    MultisamplingPasses(default_MultisamplingPasses),
    TextureCacheSize(default_TextureCacheSize),
    MaxTextureSize(default_MaxTextureSize),
    TextureCompression(default_TextureCompression),
    EmptyCacheOnMinimize(default_EmptyCacheOnMinimize),
    VSyncMode(default_VSyncMode),
    ShowFps(default_ShowFps),
    sessionSelectedCover(default_sessionSelectedCover),
    sessionSelectedConfigTab(default_sessionSelectedConfigTab),
    /****************************** Ctx Menu tab ********************************/
    CtxHidePlaylistMenu(default_CtxHidePlaylistMenu),
    CtxHideDisplayMenu(default_CtxHideDisplayMenu),
    CtxHideSelectorMenu(default_CtxHideSelectorMenu),
    CtxHideExtViewerMenu(default_CtxHideExtViewerMenu),
    CtxHideActionsMenu(default_CtxHideActionsMenu)
{
}

void ConfigData::Reset() {
  PrefsVersion = component_PREF_VER;
  /******************************* Behaviour tab *******************************/
  Filter = default_Filter;
  Group = default_Group;
  Sort = default_Sort;
  SortGroup = default_SortGroup;
  InnerSort = default_InnerSort;
  ImgNoCover = default_ImgNoCover;
  ImgLoading = default_ImgLoading;
  /***************************** Album Source tab *******************************/
  CoverFollowsPlayback = default_CoverFollowsPlayback;
  CoverFollowDelay = default_CoverFollowDelay;
  FindAsYouType = default_FindAsYouType;
  FindAsYouTypeCaseSens = default_FindAsYouTypeCaseSens;
  TargetPlaylist = default_TargetPlaylist;
  DoubleClick = default_DoubleClick;
  MiddleClick = default_MiddleClick;
  EnterKey = default_EnterKey;
  CustomActionFlag = default_CustomActionFlag;
  CustomActionHRate = default_CustomActionHRate;
  CoverFollowsLibrarySelection = default_CoverFollowsLibrarySelection;
  CoverFollowsAnonymSelection = default_CoverFollowsAnonymSelection;
  CoverFollowsPlaylistSelection = default_CoverFollowsPlaylistSelection;
  CoverHighLightPlaylistSelection = default_CoverHighLightPlaylistSelection;
  /***************************** Album Source tab v2 ****************************/
  SourcePlaylist = default_SourcePlaylist;
  SourceActivePlaylist = default_SourceActivePlaylist;
  SourcePlaylistName = default_SourcePlaylistName;
  SourcePlaylistName = default_SourceActivePlaylistName;
  SourceActivePlaylistSkipAni = default_SourceActivePlaylistSkipAni;
  SourcePlaylistGroup = default_SourcePlaylistGroup;
  SourcePlaylistNGTitle = default_SourcePlaylistNGTitle;
  SourceLibrarySelector = default_SourceLibrarySelector;
  SourceLibrarySelectorLock = default_SourceLibrarySelectorLock;
  /******************************** Display tab *********************************/
  ShowAlbumTitle = default_ShowAlbumTitle;
  AlbumTitle = default_AlbumTitle;
  TitlePosH = default_TitlePosH;
  TitlePosV = default_TitlePosV;
  TitleColorCustom = default_TitleColorCustom;
  TitleColor = default_TitleColor;
  TitleFontCustom = default_TitleFontCustom;
  TitleFont = def_cfgTitleFont();
  PanelBgCustom = default_PanelBgCustom;
  PanelBg = default_PanelBg;
  HighlightWidth = default_HighlightWidth;
  CustomCoverFrontArt = default_CustomCoverFrontArt;
  CoverArtEnablePngAlpha = default_CoverArtEnablePngAlpha;
  CoverUseLegacyExternalViewer = default_CoverUseLegacyExternalViewer;
  /***************************** Cover Display tab *****************************/
  CoverConfigSel = default_CoverConfigSel;
  /****************************** Performance tab ******************************/
  Multisampling = default_Multisampling;
  MultisamplingPasses = default_MultisamplingPasses;
  TextureCacheSize = default_TextureCacheSize;
  MaxTextureSize = default_MaxTextureSize;
  TextureCompression = default_TextureCompression;
  EmptyCacheOnMinimize = default_EmptyCacheOnMinimize;
  VSyncMode = default_VSyncMode;
  ShowFps = default_ShowFps;
  /******************************* Ctx menu tab ********************************/
  CtxHidePlaylistMenu = default_CtxHidePlaylistMenu;
  CtxHideDisplayMenu = default_CtxHideDisplayMenu;
  CtxHideSelectorMenu = default_CtxHideSelectorMenu;
  CtxHideExtViewerMenu = default_CtxHideExtViewerMenu;
  CtxHideActionsMenu = default_CtxHideActionsMenu;
  /******************************* Session tab *********************************/
  sessionSelectedCover = default_sessionSelectedCover;
  sessionSelectedConfigTab = default_sessionSelectedConfigTab;
}

void ConfigData::get_data_raw(stream_writer* p_stream, abort_callback& p_abort) {
  p_stream->write_string(PrefsVersion, p_abort);
  /******************************* Behaviour tab *******************************/
  p_stream->write_lendian_t(CoverFollowsPlayback, p_abort);
  p_stream->write_lendian_t(CoverFollowDelay, p_abort);
  p_stream->write_lendian_t(FindAsYouType, p_abort);
  p_stream->write_lendian_t(FindAsYouTypeCaseSens, p_abort);
  p_stream->write_string(TargetPlaylist, p_abort);
  p_stream->write_string(DoubleClick, p_abort);
  p_stream->write_string(MiddleClick, p_abort);
  p_stream->write_string(EnterKey, p_abort);
  p_stream->write_lendian_t(CustomActionFlag, p_abort);
  p_stream->write_lendian_t(CustomActionHRate, p_abort);
  p_stream->write_lendian_t(CoverFollowsLibrarySelection, p_abort);
  p_stream->write_lendian_t(CoverFollowsAnonymSelection, p_abort);
  p_stream->write_lendian_t(CoverFollowsPlaylistSelection, p_abort);
  p_stream->write_lendian_t(CoverHighLightPlaylistSelection, p_abort);
  /***************************** Album Source tab ******************************/
  p_stream->write_string(Filter, p_abort);
  p_stream->write_string(Group, p_abort);
  p_stream->write_string(Sort, p_abort);
  p_stream->write_lendian_t(SortGroup, p_abort);
  p_stream->write_string(InnerSort, p_abort);
  p_stream->write_string(ImgNoCover, p_abort);
  p_stream->write_string(ImgLoading, p_abort);
  /***************************** Album Source tab v.2 **************************/
  p_stream->write_lendian_t(SourcePlaylist, p_abort);
  p_stream->write_lendian_t(SourceActivePlaylist, p_abort);
  p_stream->write_string(SourcePlaylistName, p_abort);
  p_stream->write_string(SourceActivePlaylistName, p_abort);
  p_stream->write_lendian_t(SourceActivePlaylistSkipAni, p_abort);
  p_stream->write_lendian_t(SourcePlaylistGroup, p_abort);
  p_stream->write_string(SourcePlaylistNGTitle, p_abort);
  //p_stream->write_lendian_t(SourceLibrarySelector, p_abort);
  p_stream->write_lendian_t(false, p_abort);
  //p_stream->write_lendian_t(SourceLibrarySelectorLock, p_abort);
  p_stream->write_lendian_t(false, p_abort);
  /******************************** Display tab ********************************/
  p_stream->write_lendian_t(ShowAlbumTitle, p_abort);
  p_stream->write_string(AlbumTitle, p_abort);
  p_stream->write_lendian_t(TitlePosH, p_abort);
  p_stream->write_lendian_t(TitlePosV, p_abort);
  p_stream->write_lendian_t(TitleColorCustom, p_abort);
  p_stream->write_lendian_t(TitleColor, p_abort);
  p_stream->write_lendian_t(TitleFontCustom, p_abort);
  p_stream->write_object(&TitleFont, sizeof(TitleFont), p_abort);
  p_stream->write_lendian_t(PanelBgCustom, p_abort);
  p_stream->write_lendian_t(PanelBg, p_abort);
  p_stream->write_lendian_t(HighlightWidth, p_abort);
  /******************************** Display tab v.2*****************************/
  //p_stream->write_lendian_t(CenterArt, p_abort);
  p_stream->write_lendian_t(CustomCoverFrontArt, p_abort);
  p_stream->write_lendian_t(CoverArtEnablePngAlpha, p_abort);
  p_stream->write_lendian_t(CoverUseLegacyExternalViewer, p_abort);
  /***************************** Cover Display tab *****************************/
  p_stream->write_string(CoverConfigSel, p_abort);
  /****************************** Performance tab ******************************/
  p_stream->write_lendian_t(Multisampling, p_abort);
  p_stream->write_lendian_t(MultisamplingPasses, p_abort);
  p_stream->write_lendian_t(TextureCacheSize, p_abort);
  p_stream->write_lendian_t(MaxTextureSize, p_abort);
  p_stream->write_lendian_t(TextureCompression, p_abort);
  p_stream->write_lendian_t(EmptyCacheOnMinimize, p_abort);
  p_stream->write_lendian_t(VSyncMode, p_abort);
  p_stream->write_lendian_t(ShowFps, p_abort);
  /********************************* Ctx Menu **********************************/
  p_stream->write_lendian_t(CtxHidePlaylistMenu, p_abort);
  p_stream->write_lendian_t(CtxHideDisplayMenu, p_abort);
  p_stream->write_lendian_t(CtxHideSelectorMenu, p_abort);
  p_stream->write_lendian_t(CtxHideExtViewerMenu, p_abort);
  p_stream->write_lendian_t(CtxHideActionsMenu, p_abort);
  /********************************* Session ***********************************/
  p_stream->write_string(sessionSelectedCover, p_abort);
  sessionCompiledCPInfo.pub_get_data_raw(p_stream, p_abort);
  p_stream->write_lendian_t(sessionSelectedConfigTab, p_abort);
}

void ConfigData::set_data_raw(stream_reader* p_stream, t_size p_sizehint,
                              abort_callback& p_abort) {
  SetData(*this, p_stream, p_abort, component_PREF_VER);
}

void ConfigData::SetData(ConfigData& cfg, stream_reader* p_stream,
                         abort_callback& p_abort, const char* version) {
  p_stream->read_string(cfg.PrefsVersion, p_abort);

  //todo: if(cfg.PrefsVersion != version ....

  /******************************* Behaviour tab *******************************/
  p_stream->read_lendian_t(cfg.CoverFollowsPlayback, p_abort);
  p_stream->read_lendian_t(cfg.CoverFollowDelay, p_abort);
  p_stream->read_lendian_t(cfg.FindAsYouType, p_abort);
  p_stream->read_lendian_t(cfg.FindAsYouTypeCaseSens, p_abort);
  p_stream->read_string(cfg.TargetPlaylist, p_abort);
  p_stream->read_string(cfg.DoubleClick, p_abort);
  p_stream->read_string(cfg.MiddleClick, p_abort);
  p_stream->read_string(cfg.EnterKey, p_abort);
  p_stream->read_lendian_t(cfg.CustomActionFlag, p_abort);
  p_stream->read_lendian_t(cfg.CustomActionHRate, p_abort);
  p_stream->read_lendian_t(cfg.CoverFollowsLibrarySelection, p_abort);
  p_stream->read_lendian_t(cfg.CoverFollowsAnonymSelection, p_abort);
  p_stream->read_lendian_t(cfg.CoverFollowsPlaylistSelection, p_abort);
  p_stream->read_lendian_t(cfg.CoverHighLightPlaylistSelection, p_abort);
  /***************************** Album Source tab ******************************/
  p_stream->read_string(cfg.Filter, p_abort);
  p_stream->read_string(cfg.Group, p_abort);
  p_stream->read_string(cfg.Sort, p_abort);
  p_stream->read_lendian_t(cfg.SortGroup, p_abort);
  p_stream->read_string(cfg.InnerSort, p_abort);
  p_stream->read_string(cfg.ImgNoCover, p_abort);
  p_stream->read_string(cfg.ImgLoading, p_abort);
  /*************************** Album Source tab v.2 ****************************/
  p_stream->read_lendian_t(cfg.SourcePlaylist, p_abort);
  p_stream->read_lendian_t(cfg.SourceActivePlaylist, p_abort);
  p_stream->read_string(cfg.SourcePlaylistName, p_abort);
  p_stream->read_string(cfg.SourceActivePlaylistName, p_abort);
  p_stream->read_lendian_t(cfg.SourceActivePlaylistSkipAni, p_abort);
  p_stream->read_lendian_t(cfg.SourceActivePlaylistSkipAni, p_abort);
  p_stream->read_lendian_t(cfg.SourcePlaylistGroup, p_abort);
  p_stream->read_string(cfg.SourcePlaylistNGTitle, p_abort);
  p_stream->read_lendian_t(cfg.SourceLibrarySelector, p_abort);
  p_stream->read_lendian_t(cfg.SourceLibrarySelectorLock, p_abort);
  /******************************** Display tab ********************************/
  p_stream->read_lendian_t(cfg.ShowAlbumTitle, p_abort);
  p_stream->read_string(cfg.AlbumTitle, p_abort);
  p_stream->read_lendian_t(cfg.TitlePosH, p_abort);
  p_stream->read_lendian_t(cfg.TitlePosV, p_abort);
  p_stream->read_lendian_t(cfg.TitleColorCustom, p_abort);
  p_stream->read_lendian_t(cfg.TitleColor, p_abort);
  p_stream->read_lendian_t(cfg.TitleFontCustom, p_abort);
  cfg.TitleFont = def_cfgTitleFont();
  p_stream->read_object(&cfg.TitleFont, sizeof(cfg.TitleFont), p_abort);
  p_stream->read_lendian_t(cfg.PanelBgCustom, p_abort);
  p_stream->read_lendian_t(cfg.PanelBg, p_abort);
  p_stream->read_lendian_t(cfg.HighlightWidth, p_abort);
  /******************************** Display tab v.2*****************************/
  //p_stream->read_lendian_t(cfg.CenterArt, p_abort);
  p_stream->read_lendian_t(cfg.CustomCoverFrontArt, p_abort);
  p_stream->read_lendian_t(cfg.CoverArtEnablePngAlpha, p_abort);
  p_stream->read_lendian_t(cfg.CoverUseLegacyExternalViewer, p_abort);
  /***************************** Cover Display tab *****************************/
  p_stream->read_string(CoverConfigSel, p_abort);
  /****************************** Performance tab ******************************/
  p_stream->read_lendian_t(cfg.Multisampling, p_abort);
  p_stream->read_lendian_t(cfg.MultisamplingPasses, p_abort);
  p_stream->read_lendian_t(cfg.TextureCacheSize, p_abort);
  p_stream->read_lendian_t(cfg.MaxTextureSize, p_abort);
  p_stream->read_lendian_t(cfg.TextureCompression, p_abort);
  p_stream->read_lendian_t(cfg.EmptyCacheOnMinimize, p_abort);
  p_stream->read_lendian_t(cfg.VSyncMode, p_abort);
  p_stream->read_lendian_t(cfg.ShowFps, p_abort);
  /********************************* Ctx menu **********************************/
  p_stream->read_lendian_t(cfg.CtxHidePlaylistMenu, p_abort);
  p_stream->read_lendian_t(cfg.CtxHideDisplayMenu, p_abort);
  p_stream->read_lendian_t(cfg.CtxHideSelectorMenu, p_abort);
  p_stream->read_lendian_t(cfg.CtxHideExtViewerMenu, p_abort);
  p_stream->read_lendian_t(cfg.CtxHideActionsMenu, p_abort);
  /********************************* Session ***********************************/
  p_stream->read_string(cfg.sessionSelectedCover, p_abort);
  sessionCompiledCPInfo.pub_set_data_raw(p_stream, 0, p_abort);
  p_stream->read_lendian_t(cfg.sessionSelectedConfigTab, p_abort);
}

} // namespace coverflow
