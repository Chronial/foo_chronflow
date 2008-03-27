#include "chronflow.h"

extern cfg_bool cfgCoverFollowsPlayback;
extern cfg_int cfgCoverFollowDelay;

PlaybackTracer::PlaybackTracer(AppInstance* appInstance) :
appInstance(appInstance),
callbackRegistered(false),
status(ready)
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
	if (cfgCoverFollowsPlayback && (status == inUserMovement)){
		SetTimer(appInstance->mainWindow, IDT_PLAYBACK_TRACER, (cfgCoverFollowDelay * 1000), NULL);
		status = afterUserMovement;
	}
}

void PlaybackTracer::timerHit()
{
	if (status == afterUserMovement){
		moveToNowPlaying();
		KillTimer(appInstance->mainWindow, IDT_PLAYBACK_TRACER);
		status = ready;
	}
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
		moveToNowPlaying();
	} else {
		KillTimer(appInstance->mainWindow, IDT_PLAYBACK_TRACER);
		status = ready;
		if (callbackRegistered){
			static_api_ptr_t<play_callback_manager> pcm;
			pcm->unregister_callback(this);
			callbackRegistered = false;
		}
	}
}

void PlaybackTracer::on_playback_new_track(metadb_handle_ptr p_track)
{
	if (status == ready){
		moveToNowPlaying();
	}
}