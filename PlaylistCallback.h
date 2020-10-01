#pragma once
#include "EngineWindow.fwd.h"
namespace engine {

// EngineThread Playlist Callback
class PlaylistCallback : public playlist_callback_impl_base {
  void on_items_selection_change(t_size p_playlist, const bit_array& p_affected,
                                 const bit_array& p_state);

  void on_items_added(t_size p_playlist, t_size p_start,
                      const pfc::list_base_const_t<metadb_handle_ptr>& p_data,
                      const bit_array& p_selection) final;
  void on_items_removed(t_size p_playlist, const bit_array& p_mask, t_size p_old_count,
                        t_size p_new_count) final;
  void on_items_reordered(t_size p_playlist, const t_size* p_order, t_size p_count) final;
  void on_playlist_activate(t_size p_old, t_size p_new);
  void on_playlist_renamed(t_size p_index, const char* p_new_name,
                           t_size p_new_name_len) final;
  void on_playlists_removed(const bit_array& p_mask, t_size p_old_count,
                            t_size p_new_count) final;

 public:
  PlaylistCallback(EngineWindow& enginewindow) : engineWindow(enginewindow) {}

 private:
  EngineWindow& engineWindow;
};
}  // namespace engine
