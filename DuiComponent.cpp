#include "stdafx.h"

#include "ContainerWindow.h"



class dui_chronflow : public ui_element_instance {
	ui_element_config::ptr config;
	ContainerWindow window;
public:
	dui_chronflow(HWND parent, ui_element_config::ptr config, ui_element_instance_callback_ptr p_callback)
		: config(config), window(parent, p_callback) {
		
	}

	static GUID g_get_guid() {
		// {1D56881C-CA24-470c-944A-DED830F9E95D}
		static const GUID guid_foo_chronflow = { 0x1d56881c, 0xca24, 0x470c, { 0x94, 0x4a, 0xde, 0xd8, 0x30, 0xf9, 0xe9, 0x5d } };
		return guid_foo_chronflow;
	}
	static GUID g_get_subclass() { return ui_element_subclass_media_library_viewers; }

	GUID get_guid() { return dui_chronflow::g_get_guid(); }
	GUID get_subclass() { return dui_chronflow::g_get_subclass(); }

	static void g_get_name(pfc::string_base & out) { out = "Chronflow"; }
	static const char * g_get_description() { return "Displays a 3D rendering of the Album Art in your Media Library"; }

	static ui_element_config::ptr g_get_default_configuration() { return ui_element_config::g_create_empty(g_get_guid()); }

	HWND get_wnd() {
		return window.hwnd;
	};

	void set_configuration(ui_element_config::ptr config) { this->config = config; }
	ui_element_config::ptr get_configuration() { return config; }
};


class UiElement : public ui_element {
public:
	GUID get_guid() { return dui_chronflow::g_get_guid(); }
	GUID get_subclass() { return dui_chronflow::g_get_subclass(); }
	void get_name(pfc::string_base & out) { dui_chronflow::g_get_name(out); }
	ui_element_instance::ptr instantiate(HWND parent, ui_element_config::ptr cfg, ui_element_instance_callback::ptr callback) {
		PFC_ASSERT(cfg->get_guid() == get_guid());
		service_nnptr_t<dui_chronflow> item = new service_impl_t<dui_chronflow>(parent, cfg, callback);
		return item;
	}
	ui_element_config::ptr get_default_configuration() { return dui_chronflow::g_get_default_configuration(); }
	ui_element_children_enumerator_ptr enumerate_children(ui_element_config::ptr cfg) { return NULL; }
	bool get_description(pfc::string_base & out) { out = dui_chronflow::g_get_description(); return true; }
};

static service_factory_single_t<UiElement> uiElement;
