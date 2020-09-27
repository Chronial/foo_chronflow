#include "ConfigWindow.h"

namespace coverflow {

std::unordered_multimap<UINT, int> disableMap{
    // Sources
    {IDC_SOURCE_FROM_PLAYLIST, -IDC_FILTER},
    {IDC_SOURCE_FROM_PLAYLIST, -IDC_GROUP},
    {IDC_SOURCE_FROM_PLAYLIST, -IDC_SORT_GROUP}, //a->b
    {IDC_SORT_GROUP, -IDC_SORT},                 //b->c
    {IDC_SOURCE_FROM_PLAYLIST, -IDC_SORT},
    {IDC_SOURCE_FROM_PLAYLIST, -IDC_INNER_SORT},
    {IDC_SOURCE_FROM_PLAYLIST, IDC_SOURCE_FROM_ACTIVE_PLAYLIST},        //a->b
    {IDC_SOURCE_FROM_ACTIVE_PLAYLIST, -IDC_COMBO_SOURCE_PLAYLIST_NAME}, //b->c
    {IDC_SOURCE_FROM_PLAYLIST, IDC_COMBO_SOURCE_PLAYLIST_NAME},
    {IDC_SOURCE_FROM_PLAYLIST, IDC_SOURCE_PLAYLIST_NGTITLE},
    {IDC_SOURCE_FROM_PLAYLIST, IDC_SOURCE_PLAYLIST_GROUP},     //a->b
    {IDC_SOURCE_PLAYLIST_GROUP, -IDC_SOURCE_PLAYLIST_NGTITLE}, //b->c
    {IDC_SOURCE_FROM_PLAYLIST, IDC_CHECK_BEHA_COVER_FOLLOWS_PLAYLIST},
    {IDC_SOURCE_FROM_PLAYLIST, IDC_CHECK_BEHA_COVER_HIGHLIGHT_PLAYLIST},
    // Behaviour
    {IDC_FOLLOW_PLAYBACK, IDC_FOLLOW_DELAY},
    {IDC_FIND_AS_YOU_TYPE, IDC_FIND_AS_YOU_TYPE_CS},
    {IDC_CHECK_BEHA_COVER_FOLLOWS_LIBRARY, IDC_CHECK_BEHA_COVER_FOLLOWS_ANONYM},
    // Display
    {IDC_ALBUM_TITLE, IDC_ALBUM_FORMAT},
    {IDC_FONT_CUSTOM, IDC_FONT},
    {IDC_FONT_CUSTOM, IDC_FONT_PREV},
    {IDC_TEXTCOLOR_CUSTOM, IDC_TEXTCOLOR},
    {IDC_TEXTCOLOR_CUSTOM, IDC_TEXTCOLOR_PREV},
    {IDC_BG_COLOR_CUSTOM, IDC_BG_COLOR},
    {IDC_BG_COLOR_CUSTOM, IDC_BG_COLOR_PREV},
    // Performance
    {IDC_MULTI_SAMPLING, IDC_MULTI_SAMPLING_PASSES},
};
std::unordered_multimap<UINT, int> disableStateMatch{
    //if ctrl1 is disabled then ctrl2 is disabled (disableStateMatch)
    //if ctrl1 is enabled, ctrl2 is NOT necessarily enabled
    {IDC_SOURCE_FROM_ACTIVE_PLAYLIST, IDC_COMBO_SOURCE_PLAYLIST_NAME},
    {IDC_SOURCE_PLAYLIST_GROUP, IDC_SOURCE_PLAYLIST_NGTITLE},
    {IDC_SORT_GROUP, IDC_SORT},
};
ListMap multisamplingMap{
    {2, "  2"},
    {4, "  4"},
    {8, "  8"},
    {16, "16"},
};

ListMap customActionAddReplaceMap{
    {ACT_ADD, "Add"},
    {0, "Send"},
    {ACT_INSERT,"Insert"},
    //{} another operation available
};

// album_art_ids::cover_front...
ListMap customCoverFrontArtMap{
    {0, "cover_front"}, {1, "cover_back"}, {2, "disc"}, {3, "artist"},
};

} // namespace coverflow
