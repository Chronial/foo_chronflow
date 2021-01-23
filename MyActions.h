#pragma once
#include "DbAlbumCollection.h"
#include "DbAlbumInfo.h"

#define WM_MYACTIONS_CANCELED (WM_USER + 1000)
#define WM_MYACTIONS_SET_DISPLAY (WM_USER + 1001)

bool isInactivePlaylistPlayFixed(int amajor, int aminor, int arevision /*, int abuild*/);

class CustomAction {
 protected:
  explicit CustomAction(const char* actionName, bool isPlaylistAction) {
                        this->actionName = actionName;
                        this->bPlaylistAction = isPlaylistAction;
  }
  bool availablePlaylist(t_size playlist);

  void NotifyRejected(HWND hwnd);
  void DoPlayItem(t_size p_item, t_size playlist,
                  const pfc::bit_array_bittable& p_mask, int flag);

  t_size DoAddReplace(const pfc::list_base_const_t<metadb_handle_ptr>& tracks,
                      t_size playlist, int flag, t_size lastselected,
                      pfc::list_t<metadb_handle_ptr>& updateTracks,
                      bit_array_bittable& updateMask);

  int GetOnlyRated(bool onlyhighrates,
                   const pfc::list_base_const_t<metadb_handle_ptr>& tracks,
                   pfc::list_t<metadb_handle_ptr>& rated_items,
                   bit_array_bittable& updateMask);

  int CheckRate(metadb_handle_ptr track, bool hrate);

  bool SendToPlaylist(const pfc::list_base_const_t<metadb_handle_ptr>& tracks,
                      t_size playlist, int flag);

 public:

  pfc::string8 actionName;
  bool bPlaylistAction;

  virtual void run(const pfc::list_base_const_t<metadb_handle_ptr>& tracks,
                   const char* albumTitle, HWND engineWnd, int flag) = 0;
  virtual void sendconfig(HWND mwnd) = 0;

  bool isPlaylistAction() { return bPlaylistAction;}

  static bool isCustomAction(std::vector<CustomAction*> g_customActions, const char* actionname, bool playlistaction) {
    bool bres = false;
    for (auto action : boost::adaptors::reverse(g_customActions)) {
      if (action->isPlaylistAction() == playlistaction) {
        if (stricmp_utf8(action->actionName, actionname) == 0) {
          bres = true;
          break;
        }
      }
    }
    return bres;
  }
};

extern std::vector<CustomAction*> g_customActions;

void executeAction(const char* action, const ::db::AlbumInfo& album,
                   HWND enginewnd, int flag);


byte const nblock = 3;         // custom actions blocks (double click, middle click & enter)
byte const nflag = 8;          // flags
byte const ncheckboxflag = 5;  // ui checkboxes for flags

enum ActionBlocks {
  AB_DOUBLECLICK = 0,
  AB_MIDDLECLICK,
  AB_ENTER,
};

enum ActionFlags {
  ACT_PLAY = 1 << 0,           // 1 Play
  ACT_ACTIVATE = 1 << 1,       // 2 Activate (requires fb2k > v1.6.3)
  ACT_ADD = 1 << 2,            // 4 Add or replace
  ACT_HIGHLIGHT = 1 << 3,      // 8 Hightlight changes
  ACT_INSERT = 1 << 4,         // 16 Insert after selected, playing...
  ACT_RATED = 1 << 5,          // 32 Only rated
  ACT_RATED_HIGHT = 1 << 6,    // 64 Only rated over n
  ACT_TO_DO = 1 << 7           // 128 Not implemented
};

static inline void ActionFlagsToArray(std::vector<std::vector<byte>>& vec,
                                      unsigned long allflags) {
  vec.resize(nblock, std::vector<byte>(nflag, 0));
  for (int itblock = 0; itblock < nblock; itblock++) {
    uint8_t block_flag = itblock == 0 ? allflags : allflags >> (1 << (itblock + 2));
    for (int itflag = 0; itflag < nflag; itflag++) {
      uint8_t flag_mask = 1 << itflag;
      if ((block_flag & flag_mask) == flag_mask)
        vec[itblock][itflag] = 1;
    }
  }
}

static inline unsigned long ActionFlagsCalculate(int action_block, int actionflag,
                                                 bool flagisndx,
                                                 unsigned long customflags, bool enable) {
  uint8_t action_block_flag_mask;

  if (flagisndx)
    action_block_flag_mask = 1 << actionflag;
  else
    action_block_flag_mask = actionflag;

  unsigned long replaceval = action_block_flag_mask;
  if (action_block > 0) {
    replaceval <<= (1 << (action_block + 2));
  }
  if (enable)
    customflags |= replaceval;
  else
    customflags &= ~(replaceval);

  return customflags;
}

static inline uint8_t ActionGetBlockFlag(unsigned long customflags, int abEvent) {
  if (abEvent > nblock)
    return 0;

  int block = abEvent;
  uint8_t block_flag = block == 0 ? customflags : customflags >> (1 << (block + 2));
  return block_flag;
}

static inline bool BlockFlagEnabled(uint8_t block_flag, ActionFlags aflag) {
  if ((block_flag & aflag) == aflag)
    return true;
  else
    return false;
}
