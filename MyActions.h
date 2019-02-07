#pragma once
#include "DbAlbumCollection.h"

class CustomAction abstract {
protected:
	CustomAction(const char * actionName){
		this->actionName = actionName;
	}
public:
	pfc::string8 actionName;
	virtual void run(const pfc::list_base_const_t<metadb_handle_ptr> & tracks, const char * albumTitle) = 0;
};

extern CustomAction* g_customActions[4];

void executeAction(const char * action, const AlbumInfo& album);
