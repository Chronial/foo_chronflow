#pragma once

static const char* docuconfig =
R"us5lhvaf(-- Reload & Now Playing --

F5 - Reload collection
F6 - Move to now playing

-- Album Browsing --

LEFT, RIGHT, PRIOR, NEXT, HOME, END

-- Find as You Type --

F2 - Toggle case sensitive Find as you type

Find As You Type disables the panel alphanumeric hotkeys.
Search fields are determined by the Album Title format (Display Tab)

-- Playlist Covers --

F4 - Group/Ungroup album content
F8 - Set current playlist as source
F9 - Toggle Playlist Covers on/off
F10 - Toggle Active Playlist Covers on/off

-- Cover Selections --

Ctrl + F8 - Toggle Filter Selector Lock
Ctrl + F10 - Toggle Library Filter Selector

-- Library actions ---

Library actions are available while browsing the entire library.
When Playlist Covers is active the default action for Double Click
and Enter is 'Play'

Insert action modifier will position new items after existing selections,
if no selection is found, after the item currently playing.

Play action modifier in a non active playlist context will queue items.

-- Cover configurations --

Control + 0: Reset to the saved configuration
Control + 1..9: Run configuration

-- Album Art --

UP, DOWN, CTRL+UP, CTRL+DOWN

Keyboard cover art selection modifies the bitmap cache.
The selections are reset when the album collection is reloaded or when the album
cache gets invalidated.

Check Performance Tab to tune cache options.

-- Legacy External Viewer --

With this option enabled (Properties->Display) the Open External Viewer command will call
%ProgramFiles%\Windows Photo Viewer\PhotoViewer.dll instead of the Default Image Viewer.


)us5lhvaf";
