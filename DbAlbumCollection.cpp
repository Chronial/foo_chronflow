#include "chronflow.h"

extern cfg_string cfgFilter;
extern cfg_string cfgSources;

DbAlbumCollection::DbAlbumCollection(void)
{
	static_api_ptr_t<titleformat_compiler> compiler;
	service_ptr_t<titleformat_object> filterScript;
	compiler->compile(filterScript, cfgFilter);
	pfc::list_t<service_ptr_t<titleformat_object>> sourceScripts;
	const char * srcStart = cfgSources.get_ptr();
	const char * srcP = srcStart;
	for (;;srcP++){
		if (*srcP == '\r' || *srcP == '\n' || *srcP == '\0'){
			if (srcP-1 > srcStart){
				pfc::string8_fastalloc src;
				src.set_string(srcStart, srcStart - srcP - 1);
				service_ptr_t<titleformat_object> srcScript;
				compiler->compile(srcScript, src);
				sourceScripts.add_item(srcScript);
			}
			if (*srcP == '\0')
				break;
			srcStart = srcP+1;
		}
	}

	metadb_handle_list library;
	static_api_ptr_t<library_manager> lm;
	lm->get_all_items(library);
	library.sort_by_format(filterScript, 0);
	t_size count = library.get_count();

	bool preHide = true;
	bool postHide = false;
	pfc::string8_fastalloc filterOut;
	pfc::string8_fastalloc previousSrc;
	pfc::string8_fastalloc sourceOut;
	filterOut.prealloc(512);
	previousSrc.prealloc(512);
	sourceOut.prealloc(512);
	int sourceCount = sourceScripts.get_count();
	abort_callback_impl abortCallback;
	for (t_size i=0; i < count; i++){
		const metadb_handle_ptr f = library.get_item_ref(i);
		if (!postHide){
			f->format_title(0, filterOut, filterScript, 0);
			if (strcmp(filterOut,"!hide") == 0){
				if (preHide)
					preHide = false;
				continue;
			} else {
				if (!preHide)
					postHide = true;
			}
		}
		
		bool imgFound = false;
		for (int j=0; j < sourceCount; j++){
			f->format_title(0, sourceOut, sourceScripts.get_item_ref(j), 0);
			if (stricmp_utf8(sourceOut,previousSrc) == 0){
				imgFound = true;
				break;
			}
			try {
				if (filesystem::g_exists(sourceOut, abortCallback) &&
					!filesystem::g_is_valid_directory(sourceOut, abortCallback) &&
					!filesystem::g_is_remote_or_unrecognized(sourceOut)){
						previousSrc = sourceOut;
						imageList.add_item(sourceOut);
						imgFound = true;
				}
			} catch (exception_io_no_handler_for_path){
			}
		}
		if (!imgFound){
			// No Cover Image???
		}
	}
}

DbAlbumCollection::~DbAlbumCollection(void)
{
}

int DbAlbumCollection::getCount(){
	return imageList.get_count();
}

char* DbAlbumCollection::getTitle(CollectionPos pos){
	return "Titles not implemented";
}

ImgTexture* DbAlbumCollection::getImgTexture(CollectionPos pos){
	return new ImgFileTexture(pfc::stringcvt::string_wide_from_utf8(imageList[pos.toIndex()]));
}