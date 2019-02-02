#include "stdafx.h"
#include "base.h"
#include "config.h"

#include "PlaybackTracer.h"

#include "AppInstance.h"
#include "DbAlbumCollection.h"
#include "RenderThread.h"


PlaybackTracer::PlaybackTracer(AppInstance* appInstance) :
TimerOwner(appInstance),
appInstance(appInstance)
{
	followSettingsChanged();
}

PlaybackTracer::~PlaybackTracer() {
	if (callbackRegistered){
		static_api_ptr_t<play_callback_manager> pcm;
		pcm->unregister_callback(this);
	}
}

void PlaybackTracer::movementEnded()
{
	if (inMovement){
		inMovement = false;
		unlock();
	}
}
void PlaybackTracer::lock()
{
	lockCount++;
}

void PlaybackTracer::unlock()
{
	lockCount--;
	PFC_ASSERT(lockCount >= 0);
	if (lockCount == 0 && cfgCoverFollowsPlayback){
		setTimer(cfgCoverFollowDelay * 1000);
		waitingForTimer = true;
	}
}

void PlaybackTracer::timerProc()
{
	if (lockCount == 0 && cfgCoverFollowsPlayback){
		moveToNowPlaying();
	}
	waitingForTimer = false;
	killTimer();
}

void PlaybackTracer::moveToNowPlaying()
{
	fb2k::inMainThread2([&]{
		static_api_ptr_t<playback_control_v2> pc;
		metadb_handle_ptr nowPlaying;
		if (pc->get_now_playing(nowPlaying)){
			CollectionPos target;
			appInstance->renderer->send<RenderThread::MoveToTrack>(nowPlaying);
		}
	});
}

void PlaybackTracer::followSettingsChanged()
{
	if (cfgCoverFollowsPlayback){
		if (!callbackRegistered){
			static_api_ptr_t<play_callback_manager> pcm;
			pcm->register_callback(this, flag_on_playback_new_track, true);
			callbackRegistered = true;
		}
		if (lockCount == 0){
			moveToNowPlaying();
		}
	} else {
		killTimer();
		if (callbackRegistered){
			static_api_ptr_t<play_callback_manager> pcm;
			pcm->unregister_callback(this);
			callbackRegistered = false;
		}
	}
}

void PlaybackTracer::on_playback_new_track(metadb_handle_ptr p_track)
{
	if (lockCount == 0 && !waitingForTimer){
		moveToNowPlaying();
	}
}