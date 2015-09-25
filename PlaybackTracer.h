#pragma once
#include "stdafx.h"

class AppInstance;

class PlaybackTracer : public play_callback
{
private:
	AppInstance* appInstance;

public:
	PlaybackTracer(AppInstance* instance);
	~PlaybackTracer();

	void userStartedMovement()
	{
		if (!inMovement){
			inMovement = true;
			lock();
		}
	}
	void lock();
	void unlock();

	void userAction()
	{
		lock();
		unlock();
	}

	void movementEnded();
	void timerHit();
	void followSettingsChanged();

	void moveToNowPlaying();

private:
	bool inMovement;
	bool waitingForTimer;
	bool callbackRegistered;
	int lockCount;


public:
	//! Playback advanced to new track.
	void on_playback_new_track(metadb_handle_ptr p_track);

	void on_playback_starting(play_control::t_track_command p_command,bool p_paused){};
	void on_playback_stop(play_control::t_stop_reason p_reason){};
	void on_playback_seek(double p_time){};
	void on_playback_pause(bool p_state){};
	void on_playback_edited(metadb_handle_ptr p_track){};
	void on_playback_dynamic_info(const file_info & p_info){};
	void on_playback_dynamic_info_track(const file_info & p_info){};
	void on_playback_time(double p_time){};
	void on_volume_change(float p_new_val){};
};

class PlaybackTracerScopeLock {
public:
	PlaybackTracerScopeLock(PlaybackTracer &tracer)
		: t(&tracer){
			t->lock();
	}
	~PlaybackTracerScopeLock(){
		t->unlock();
	}
private:
	PlaybackTracer* t;
};