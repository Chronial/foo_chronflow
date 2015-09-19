#pragma once
#include "resource.h"

#ifdef _DEBUG
#define IF_DEBUG(X) X
#else
#define IF_DEBUG(X)  
#endif


#ifdef _DEBUG
#define BOOST_MULTI_INDEX_ENABLE_SAFE_MODE
#endif


#ifndef M_PI
#define M_PI 3.1415926535897932385
#endif
#define deg2rad(X) (X*M_PI/180)
#define rad2deg(X) (X*180/M_PI)

#define WM_COLLECTION_REFRESHED (WM_USER+1)

#define IDT_CHECK_MINIMIZED 665
#define IDT_PLAYBACK_TRACER 666
#define IDT_FIND_AS_YOU_TYPE_RESET 667

// TODO: This is dirty, do it right
class AppInstance;
void getAppInstances(pfc::array_t<AppInstance*> &instances);
#define FOR_EACH_INSTANCE(call)	{pfc::array_t<AppInstance*> instances; getAppInstances(instances); for (t_size i = 0; i < instances.get_count(); i++) { instances[i]->call; }}



// {37835416-4578-4aaa-A229-E09AB9E2CB9C}
const GUID guid_configWindow = { 0x37835416, 0x4578, 0x4aaa, { 0xa2, 0x29, 0xe0, 0x9a, 0xb9, 0xe2, 0xcb, 0x9c } };
