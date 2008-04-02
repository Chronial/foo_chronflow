#include "chronflow.h"

extern cfg_bool cfgCoverFollowsPlayback;
extern cfg_int cfgCoverFollowDelay;

PlaybackTracer::PlaybackTracer(AppInstance* appInstance) :
appInstance(appInstance),
callbackRegistered(false),
inMovement(false),
waitingForTimer(false),
lockCount(0)
{
	followSettingsChanged();
}

PlaybackTracer::~PlaybackTracer() {
	if (callbackRegistered){
		static_api_ptr_t<play_callback_manager> pcm;
		pcm->unregister_callback(this);
	}
	if (appInstance->mainWindow)
		KillTimer(appInstance->mainWindow, IDT_PLAYBACK_TRACER);
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
	IF_DEBUG(PFC_ASSERT(lockCount >= 0));
	if (lockCount == 0 && cfgCoverFollowsPlayback){
		SetTimer(appInstance->mainWindow, IDT_PLAYBACK_TRACER, (cfgCoverFollowDelay * 1000), NULL);
		waitingForTimer = true;
	}
}

void PlaybackTracer::timerHit()
{
	if (lockCount == 0 && cfgCoverFollowsPlayback){
		moveToNowPlaying();
	}
	waitingForTimer = false;
	KillTimer(appInstance->mainWindow, IDT_PLAYBACK_TRACER);
}

void PlaybackTracer::moveToNowPlaying()
{
	static_api_ptr_t<playback_control_v2> pc;
	metadb_handle_ptr nowPlaying;
	if (pc->get_now_playing(nowPlaying)){
		CollectionPos target = appInstance->displayPos->getTarget();
		if (appInstance->albumCollection->getAlbumForTrack(nowPlaying, target)){
			appInstance->displayPos->setTarget(target);
		}
	}
}

void PlaybackTracer::followSettingsChanged()
{
	if (cfgCoverFollowsPlayback){
		if (!callbackRegistered){
			static_api_ptr_t<play_callback_manager> pcm;
			pcm->register_callback(this, flag_on_playback_new_track, true);
			callbackRegistered = true;
		}
		if (lockCount == 0)
			moveToNowPlaying();
	} else {
		KillTimer(appInstance->mainWindow, IDT_PLAYBACK_TRACER);
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