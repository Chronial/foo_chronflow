#pragma once
// clang-format off
#include "EngineWindow.fwd.h"
#include "style_manager.h"

#include "cover_positions_compiler.h"
#include "utils.h"
#include "ConfigData.h"

#include "COM_Guid.h"
#include "COM_Module.h"
#include "COM_ClassFactory.h"

// clang-format off

namespace engine {

using render::StyleManager;
using coverflow::configData;

struct GdiContext {
  ULONG_PTR token{};
  GdiContext() {
    Gdiplus::GdiplusStartupInput input;
    Gdiplus::GdiplusStartup(&token, &input, nullptr);
  }
  GdiContext(const GdiContext&) = delete;
  GdiContext& operator=(const GdiContext&) = delete;
  GdiContext(const GdiContext&&) = delete;
  GdiContext& operator=(GdiContext&&) = delete;
  ~GdiContext() { Gdiplus::GdiplusShutdown(token); }
};

class ContainerWindow {
 public:
  explicit ContainerWindow(HWND parent, StyleManager& styleManager,
                           ui_element_instance_callback_ptr duiCallback = nullptr);
  NO_MOVE_NO_COPY(ContainerWindow);
  ~ContainerWindow();
  void destroyEngineWindow(std::string errorMessage);
  HWND getHWND() const { return hwnd; }
  HWND getEngineWnd() const;

  //x ui ------------------
  
  //host to dlg
  void set_uicfg(stream_reader_formatter<>* data, t_size p_size, abort_callback & p_abort); 
  //dlg to host
  void get_uicfg(stream_writer_formatter<>* data, abort_callback & p_abort) const;

  bool IsSourceOnAndMirrored(bool fromActive) const {
    if (fromActive)
      return (coverEnableActivePlaylistCovers &&
            (!stricmp_utf8(coverSourceActivePlaylistName, coverSourcePlaylistName)));
    else
      return (coverEnablePlaylistCovers &&
              (!stricmp_utf8(coverSourceActivePlaylistName, coverSourcePlaylistName)));
  }

  t_size FindSourcePlaylist(PlSrcFilter mode) const {
    static_api_ptr_t<playlist_manager_v5> pm;
    if (coverIsWholeLibrary()) {
      return pfc::infinite_size;
    }
    switch (mode) {
    case PlSrcFilter::ANY_PLAYLIST:
      if (coverEnableActivePlaylistCovers) {
        return pm->find_playlist(coverSourceActivePlaylistName);
      } else
        return pm->find_playlist(coverSourcePlaylistName);
      break;
    case PlSrcFilter::PLAYLIST:
      if (coverEnablePlaylistCovers)
        return pm->find_playlist(coverSourcePlaylistName);
      break;
    case PlSrcFilter::ACTIVE_PLAYLIST:
      if (coverEnableActivePlaylistCovers)
        return pm->find_playlist(coverSourceActivePlaylistName);
      break;
    }
    return pfc::infinite_size;
  }

  bool IsSourcePlaylistOn(t_size playlist, PlSrcFilter mode) {
    const t_size srcplaylist = FindSourcePlaylist(mode);
    return (srcplaylist != ~0 && srcplaylist == playlist);
  }

  void GetStateTrackInfo(t_size &trackpos, metadb_handle_ptr & track) {
    const t_size plsource = FindSourcePlaylist(PlSrcFilter::ANY_PLAYLIST);
    static_api_ptr_t<playlist_manager> pm;
    if (pm->playlist_get_item_count(plsource)) {
        bit_array_bittable selmask;
        pm->playlist_get_selection_mask(plsource, selmask);
        trackpos = selmask.find_first(true, 0, selmask.size());
        if (selmask.size() > 0 && trackpos != pfc_infinite) {
          metadb_handle_list msl;
          track = pm->playlist_get_item_handle(plsource, trackpos);
        }
        else {
          if (pm->playlist_get_focus_item_handle(track, plsource)) {
            trackpos = pm->playlist_get_focus_item(plsource);
          } else {
            track = pm->playlist_get_item_handle(plsource, 0);
            trackpos = 0;
          }
        }
    } else
      trackpos = pfc::infinite_size;
  }

  void GetState(src_state& srcstate, bool init = true) {
    if (init) {
      srcstate.wholelib.first = !(coverEnablePlaylistCovers || coverEnableActivePlaylistCovers);
      srcstate.active_pl_src.first = coverEnableActivePlaylistCovers;
      srcstate.pl_src.first = coverEnablePlaylistCovers;
      srcstate.grouped.first = coverEnablePlaylistCoversGrouped;
      if (!srcstate.wholelib.first) {
        GetStateTrackInfo(srcstate.track_first.first, srcstate.track_first.second);
      }
    } else {
      srcstate.wholelib.second = !(coverEnablePlaylistCovers || coverEnableActivePlaylistCovers);
      srcstate.active_pl_src.second = coverEnableActivePlaylistCovers;
      srcstate.pl_src.second = coverEnablePlaylistCovers;
      srcstate.grouped.second = coverEnablePlaylistCoversGrouped;
      if (!srcstate.wholelib.second) {
        GetStateTrackInfo(srcstate.track_second.first, srcstate.track_second.second);
      }
    }
  }

  bool coverIsWholeLibrary() const { return !(coverEnablePlaylistCovers || coverEnableActivePlaylistCovers); }

  pfc::string8 InSourePlaylistGetName() {
    if (!coverIsWholeLibrary()) {
      return coverEnableActivePlaylistCovers ? coverSourceActivePlaylistName : coverSourcePlaylistName;
    } else {
      return {};
    }
  }

  void SetCoverArt(size_t cover_art) {
    coverArt = cover_art;
  }

  size_t GetCoverArt() {
    return coverArt;
  }

  void ResetCoverDisp() {
    SetDisplayFlag(configData->DisplayFlag);
    SetSourcePlaylistGUID(configData->SourcePlaylistGUID);
    SetSourcePlaylistName(configData->SourcePlaylistName);
    SetSourceActivePlaylistGUID(configData->SourceActivePlaylistGUID);
    SetSourceActivePlaylistName(configData->SourceActivePlaylistName);

    size_t dummy_ndx = pfc::infinite_size;
    SetCoverConfigNdx(dummy_ndx);
    auto ndx = GetCoverConfigNdx();

    ApplyCoverConfig(true, ndx);
  }

  void SetDisplayFlag(size_t display_flag) {
    coverDisplayFlag = display_flag;
    coverSetsLibrarySel = display_flag & DispFlags::SET_LIB_SEL;
    coverSetsPlaylistSel = display_flag & DispFlags::SET_PL_SEL;
    coverFollowLibrarySel = display_flag & DispFlags::FOLLOW_LIB_SEL;
    coverFollowPlaylistSel = display_flag & DispFlags::FOLLOW_PL_SEL;
    coverFollowNowPlaying = display_flag & DispFlags::FOLLOW_PLAY_NOW;
    coverEnablePlaylistCovers = display_flag & DispFlags::SRC_PLAYLIST;
    coverEnableActivePlaylistCovers = display_flag & DispFlags::SRC_ACTPLAYLIST;
    coverEnablePlaylistCoversGrouped = !(display_flag & DispFlags::SRC_PL_UNGROUPED);
    coverEnablePlaylistCoversHiLight = display_flag & DispFlags::SRC_PL_HL;
  }
  
  size_t GetDisplayFlag() const {
    size_t retval = 0;
    retval |= coverSetsLibrarySel ? DispFlags::SET_LIB_SEL : 0;
    retval |= coverFollowLibrarySel ? DispFlags::FOLLOW_LIB_SEL  : 0;
    retval |= coverSetsPlaylistSel ? DispFlags::SET_PL_SEL : 0;
    retval |= coverFollowPlaylistSel ? DispFlags::FOLLOW_PL_SEL : 0;
    retval |= coverFollowNowPlaying ? DispFlags::FOLLOW_PLAY_NOW : 0;
    retval |= coverEnablePlaylistCovers ? DispFlags::SRC_PLAYLIST : 0;
    retval |= coverEnableActivePlaylistCovers ? DispFlags::SRC_ACTPLAYLIST : 0;
    retval |= !coverEnablePlaylistCoversGrouped ? DispFlags::SRC_PL_UNGROUPED : 0;
    retval |= coverEnablePlaylistCoversHiLight ? DispFlags::SRC_PL_HL : 0;
    return retval;
  }

  void SetCoverDispFlagU(DispFlags flg, bool enabled) {
    if (enabled) {
      coverDisplayFlag |= flg;
    }
    else {
      coverDisplayFlag &= ~flg;
    }
    SetDisplayFlag(coverDisplayFlag);
  }

  bool ToggleCoverDispFlagU(DispFlags flg) {
    coverDisplayFlag ^= flg;
    SetDisplayFlag(coverDisplayFlag);
    return coverDisplayFlag & flg;
  }

  bool GetCoverDispFlagU(DispFlags flg) {
    return coverDisplayFlag & flg;
  }

  void SetCoverFollowNowPlaying(bool cover_follow_now_playing) {

    if (cover_follow_now_playing) {
      coverDisplayFlag |= DispFlags::FOLLOW_PLAY_NOW;
    }
    else {
      coverDisplayFlag &= ~DispFlags::FOLLOW_PLAY_NOW;
    }

    coverFollowNowPlaying = cover_follow_now_playing;
  }
  bool GetCoverFollowNowPlaying() {

    return coverFollowNowPlaying;
  }

  void SetCoverConfigNdx(size_t& config_ndx) {
    if (config_ndx >= configData->CoverConfigs.size())
    {
      int default_pos;
      if ((default_pos = configData->GetCCPosition(configData->CoverConfigSel/*coverConfigScript_Default*/)) == -1) {
        config_ndx = configData->CoverConfigs.size() - 1;
      }
      else {
        config_ndx = static_cast<size_t>(default_pos);
      }
    }    
    coverConfigScript_ndx = config_ndx;
  }

  size_t GetCoverConfigNdx() {
    if (coverConfigScript_ndx == pfc::infinite_size) {
      int default_pos = configData->GetCCPosition(defaultCoverConfig);
      if ((default_pos = configData->GetCCPosition(configData->CoverConfigSel/*coverConfigScript_Default*/)) == -1) {
        //todo: rev prob unnecessary
        coverConfigScript_ndx = configData->CoverConfigs.size() - 1;
      }
      else {
        coverConfigScript_ndx = static_cast<size_t>(default_pos);
      }
    }
    //coverConfigScript_ndx = std::min(configData->CoverConfigs.size() - 1, coverConfigScript_ndx);
    return coverConfigScript_ndx;
  }

  size_t GetConstCoverConfigNdx() const {
    int config_ndx;
    if (coverConfigScript_ndx == pfc::infinite_size) {
      if ((config_ndx = configData->GetCCPosition(coverConfigScript_Sel)) == -1) {
        return configData->CoverConfigs.size() - 1;
      }
      else {
        return static_cast<size_t>(config_ndx);
      }
    }
    //coverConfigScript_ndx = std::min(configData->CoverConfigs.size() - 1, coverConfigScript_ndx);
    return coverConfigScript_ndx;
  }

  pfc::string8 GetSourcePlaylistGUID() const { return coverSourcePlaylistGUID; }
  pfc::string8 GetSourcePlaylistName() const { return coverSourcePlaylistName; }
  pfc::string8 GetSourceActivePlaylistGUID() const {return coverSourceActivePlaylistGUID; }
  pfc::string8 GetSourceActivePlaylistName() const {return coverSourceActivePlaylistName; }
  void SetSourcePlaylistGUID(pfc::string8 val) { coverSourcePlaylistGUID = val; }
  void SetSourcePlaylistName(pfc::string8 val) { coverSourcePlaylistName = val; }
  void SetSourceActivePlaylistGUID(pfc::string8 val) { coverSourceActivePlaylistGUID = val; }
  void SetSourceActivePlaylistName(pfc::string8 val) { coverSourceActivePlaylistName = val; }

  pfc::string8 GetCoverConfigDefault() { return configData->CoverConfigSel/*coverConfigScript_Default*/; }
  void ApplyCoverConfig(bool bcompile, size_t ndx = ~0);

  //end x ui --------------

 private:

  HWND createWindow(HWND parent);
  static LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);
  LRESULT MessageHandler(UINT msg, WPARAM wp, LPARAM lp);
  void drawErrorMessage();
  bool ensureScriptControlVersion();
  void ensureIsSet(int listposition, shared_ptr<CompiledCPInfo>& sessionCompiledCPInfo);

  GdiContext gdiContext;
  bool mainWinMinimized = true;
  std::string engineError{};
  // Note that the HWND might be destroyed by its parent via DestroyWindow()
  // before this class is destroyed.
  HWND hwnd = nullptr;
  
  std::unique_ptr<EngineWindow> engineWindow;
  CoverflowClassFactory m_coverflowClassFactory;
  DWORD m_coverflowClassFactoryRegID = 0;

  //x ui ------------------
  //todo:
  t_size coverArt = pfc::infinite_size;

  t_size coverConfigScript_ndx = configData->GetCCPosition(defaultCoverConfig);
  //todo_ rev depricate, use only ndx
  pfc::string8 coverConfigScript_Sel = defaultCoverConfig;
  //todo: rev depricate, use glb
  pfc::string8 coverConfigScript_Default = defaultCoverConfig;
  t_size coverDisplayFlag = pfc::infinite_size;

  bool coverShowAlbumTitle;

  bool coverSetsLibrarySel = false;
  bool coverFollowNowPlaying = false;
  bool coverFollowLibrarySel = false;

  bool coverSetsPlaylistSel = false;
  bool coverFollowPlaylistSel = false;

  bool coverEnablePlaylistCovers = false;
  bool coverEnableActivePlaylistCovers = false;

  bool coverEnablePlaylistCoversGrouped = true;
  bool coverEnablePlaylistCoversHiLight = false;

  pfc::string8 coverSourcePlaylistGUID;
  pfc::string8 coverSourcePlaylistName;
  pfc::string8 coverSourceActivePlaylistGUID;
  pfc::string8 coverSourceActivePlaylistName;

  //end x ui --------------

};
} // namespace
