#include "stdafx.h"
#include "config.h"

#include "MyActions.h"


namespace {
	class AddToPlaylist : public CustomAction {
	public:
		AddToPlaylist() : CustomAction("Add to Active Playlist "){}
		void run(const pfc::list_base_const_t<metadb_handle_ptr> & tracks, const char * albumTitle){
			static_api_ptr_t<playlist_manager> pm;
			pm->activeplaylist_undo_backup();
			t_size oldLength = pm->activeplaylist_get_item_count();
			pm->activeplaylist_add_items(tracks, bit_array_true());
			pm->activeplaylist_set_selection(bit_array_true(), bit_array_range(oldLength, tracks.get_count()));
			pm->activeplaylist_ensure_visible(oldLength + tracks.get_count()-1);
			pm->activeplaylist_set_focus_item(oldLength);
		}
	};
	class ReplacePlaylist : public CustomAction {
	public:
		ReplacePlaylist() : CustomAction("Replace Active Playlist "){}
		void run(const pfc::list_base_const_t<metadb_handle_ptr> & tracks, const char * albumTitle){
			static_api_ptr_t<playlist_manager> pm;
			pm->activeplaylist_undo_backup();
			pm->activeplaylist_clear();
			pm->activeplaylist_add_items(tracks, bit_array_true());
			t_size playlist = pm->get_active_playlist();
			pm->set_playing_playlist(playlist);
			static_api_ptr_t<playback_control> pc;
			pc->start();
		}
	};
	class NewPlaylist : public CustomAction {
	public:
		NewPlaylist() : CustomAction("Add to Album Playlist "){}
		void run(const pfc::list_base_const_t<metadb_handle_ptr> & tracks, const char * albumTitle){
			static_api_ptr_t<playlist_manager> pm;
			t_size playlist = pm->find_playlist(albumTitle);
			if (playlist != ~0){
				pm->playlist_undo_backup(playlist);
				pm->playlist_clear(playlist);
			} else {
				playlist = pm->create_playlist(albumTitle, ~0u, ~0u);
			}
			pm->playlist_add_items(playlist,tracks,bit_array_true());
			pm->set_active_playlist(playlist);
			pm->set_playing_playlist(playlist);
			static_api_ptr_t<playback_control> pc;
			pc->start();
		}
	};
	class TargetPlaylist : public CustomAction {
	public:
		TargetPlaylist() : CustomAction("Replace Default Playlist "){}
		void run(const pfc::list_base_const_t<metadb_handle_ptr> & tracks, const char * albumTitle){
			static_api_ptr_t<playlist_manager> pm;
			t_size playlist = pm->find_playlist(cfgTargetPlaylist);
			if (playlist != ~0){
				pm->playlist_undo_backup(playlist);
				pm->playlist_clear(playlist);
			} else {
				playlist = pm->create_playlist(cfgTargetPlaylist, ~0u, ~0u);
			}
			pm->playlist_add_items(playlist,tracks,bit_array_true());
			pm->set_active_playlist(playlist);
			pm->set_playing_playlist(playlist);
			static_api_ptr_t<playback_control> pc;
			pc->start();
		}
	};
};


CustomAction* g_customActions[4] = {new AddToPlaylist,
									new ReplacePlaylist,
									new TargetPlaylist,
									new NewPlaylist};



void executeAction(const char * action, const AlbumInfo& album) {
	for (int i = 0; i < tabsize(g_customActions); i++) {
		if (stricmp_utf8(action, g_customActions[i]->actionName) == 0) {
			g_customActions[i]->run(album.tracks, album.title.c_str());
			return;
		}
	}
	GUID commandGuid;
	if (menu_helpers::find_command_by_name(action, commandGuid)) {
		menu_helpers::run_command_context(commandGuid, pfc::guid_null, album.tracks);
	}
}