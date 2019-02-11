#pragma once
#include "DbAlbumCollection.h"

class CustomAction {
 protected:
  explicit CustomAction(const char* actionName) { this->actionName = actionName; }

 public:
  pfc::string8 actionName;
  virtual void run(const pfc::list_base_const_t<metadb_handle_ptr>& tracks,
                   const char* albumTitle) = 0;
};

extern std::vector<CustomAction*> g_customActions;

void executeAction(const char* action, const AlbumInfo& album);
