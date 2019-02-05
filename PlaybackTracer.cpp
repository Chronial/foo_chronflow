#include "stdafx.h"
#include "base.h"
#include "config.h"

#include "PlaybackTracer.h"

#include "DbAlbumCollection.h"
#include "RenderThread.h"
#include "Engine.h"


PlaybackTracer::PlaybackTracer(RenderThread& thread) : thread(thread) {};

void PlaybackTracer::delay(double extra_time)
{
	delayTimer.emplace(cfgCoverFollowDelay + extra_time, [&] {
		this->thread.send<EM::Run>([&] {
			if (cfgCoverFollowsPlayback)
				moveToNowPlaying();
			delayTimer.reset();
		});
	});
}

void PlaybackTracer::moveToNowPlaying()
{
	this->thread.runInMainThread([&]{
		metadb_handle_ptr nowPlaying;
		if (playback_control_v2::get()->get_now_playing(nowPlaying)){
			this->thread.send<EM::MoveToCurrentTrack>(nowPlaying);
		}
	});
}

void PlaybackTracer::onPlaybackNewTrack(metadb_handle_ptr p_track)
{
	if (!cfgCoverFollowsPlayback || delayTimer.has_value())
		return;
	moveToNowPlaying();
}