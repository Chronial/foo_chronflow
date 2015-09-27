#include "stdafx.h"
#include "base.h"

#include "AppInstance.h"
#include "DbReloadWorker.h"
#include "RenderThread.h"


void DbReloadWorker::startNew(AppInstance* instance){
	auto worker = instance->reloadWorker.synchronize();
	if (*worker) // already running
		return;
	(*worker) = unique_ptr<DbReloadWorker>(new DbReloadWorker(instance));
}

DbReloadWorker::DbReloadWorker(AppInstance* instance) : appInstance(instance){
	// copy whole library
	static_api_ptr_t<library_manager> lm;
	lm->get_all_items(library);

	//workerThread = (HANDLE)_beginthreadex(0, 0, &(this->runWorkerThread), (void*)this, 0, 0);
	thread = (HANDLE)_beginthreadex(0, 0, [](void* self)  -> unsigned int {
		static_cast<DbReloadWorker*>(self)->threadProc();
		return 0;
	}, (void*)this, 0, 0);
	SetThreadPriority(thread, THREAD_PRIORITY_BELOW_NORMAL);
	SetThreadPriorityBoost(thread, true);
};

DbReloadWorker::~DbReloadWorker(){
	kill = true;
	WaitForSingleObject(thread, INFINITE);
	CloseHandle(thread);
}

void DbReloadWorker::threadProc(){
	this->generateData();
	if (!kill)
		appInstance->renderer->send(make_shared<RTCollectionReloadedMessage>());
	// Notify renderer to copy data, after copying, refresh AsynchTexloader + start Loading
};

void DbReloadWorker::generateData(){
	console::timer_scope timer("foo_chronflow Collection Generation");
	static_api_ptr_t<titleformat_compiler> compiler;
	static_api_ptr_t<metadb> db;


	if (!cfgFilter.is_empty()){
		try {
			search_filter::ptr filter = static_api_ptr_t<search_filter_manager>()->create(cfgFilter);
			pfc::array_t<bool> mask;
			mask.set_size(library.get_count());
			filter->test_multi(library, mask.get_ptr());
			library.filter_mask(mask.get_ptr());
		}
		catch (pfc::exception const &) {};
	}

	if (kill) return;

	auto &groupIndex = albums.get<0>();
	{
		compiler->compile_safe_ex(albumMapper, cfgGroup);
		pfc::string8_fast_aggressive tmpSortString;
		albums.reserve(library.get_size());
		service_ptr_t<titleformat_object> sortFormatter;
		if (!cfgSortGroup){
			compiler->compile_safe(sortFormatter, cfgSort);
		}
		for (t_size i = 0; i < library.get_size(); i++){
			if (kill) return;
			pfc::string8_fast groupString;
			metadb_handle_ptr track = library.get_item(i);
			track->format_title(0, groupString, albumMapper, 0);
			if (!groupIndex.count(groupString.get_ptr())){
				std::wstring sortString;
				if (cfgSortGroup){
					sortString = pfc::stringcvt::string_wide_from_utf8(groupString);
				} else {
					track->format_title(0, tmpSortString, sortFormatter, 0);
					sortString = pfc::stringcvt::string_wide_from_utf8(tmpSortString);
				}

				pfc::string8_fast findAsYouType;
				track->format_title(0, findAsYouType, cfgAlbumTitleScript, 0);

				metadb_handle_list tracks;
				tracks.add_item(track);
				albums.insert({ groupString.get_ptr(), std::move(sortString), std::move(findAsYouType), std::move(tracks) });
			} else {
				auto album = groupIndex.find(groupString.get_ptr());
				album->tracks.add_item(track);
			}
		}
	}
	library.remove_all();
};
