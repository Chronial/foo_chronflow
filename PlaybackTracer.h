#pragma once
#include "stdafx.h"

#include "Helpers.h"

class PlaybackTracer
{
	class RenderThread& thread;

public:
	PlaybackTracer(RenderThread&);
	void delay(double extra_time);
	void moveToNowPlaying();
	void onPlaybackNewTrack(metadb_handle_ptr p_track);

private:
	std::optional<Timer> delayTimer;
};