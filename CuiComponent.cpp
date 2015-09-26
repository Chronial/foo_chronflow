#include "stdafx.h"
#include "../columns_ui-sdk/ui_extension.h"

#include "AppInstance.h"
#include "ChronflowWindow.h"


class cui_chronflow : public ui_extension::window, ChronflowWindow {
	ui_extension::window_host_ptr m_host;

public:
	cui_chronflow(){};

	void get_category(pfc::string_base & out) const { out = "Panels"; };

	const GUID & get_extension_guid() const {
		// {DA317C70-654F-4A75-A4F2-10F4873773FC}
		static const GUID guid_foo_chronflow = { 0xda317c70, 0x654f, 0x4a75, { 0xa4, 0xf2, 0x10, 0xf4, 0x87, 0x37, 0x73, 0xfc } };
		return guid_foo_chronflow;
	};

	void get_name(pfc::string_base & out) const { out = "Chronflow"; };

	unsigned get_type() const { return ui_extension::type_panel; };


	bool is_available(const ui_extension::window_host_ptr & p_host) const {
		return !p_host.is_valid() || !m_host.is_valid() || p_host->get_host_guid() != m_host->get_host_guid();
	}

	HWND create_or_transfer_window(HWND wnd_parent, const ui_extension::window_host_ptr &p_host, const ui_helpers::window_position_t &p_position){
		if (appInstance == nullptr) {
			m_host = p_host;
			startup(wnd_parent);
			ShowWindow(appInstance->mainWindow, SW_HIDE);
			SetWindowPos(appInstance->mainWindow, NULL, p_position.x, p_position.y, p_position.cx, p_position.cy, SWP_NOZORDER);
		} else {
			ShowWindow(appInstance->mainWindow, SW_HIDE);
			SetParent(appInstance->mainWindow, wnd_parent);
			SetWindowPos(appInstance->mainWindow, NULL, p_position.x, p_position.y, p_position.cx, p_position.cy, SWP_NOZORDER);
			m_host->relinquish_ownership(appInstance->mainWindow);
			m_host = p_host;
		}

		return appInstance->mainWindow;
	}


	void destroy_window(){
		if (appInstance != nullptr) {
			DestroyWindow(appInstance->mainWindow);
		}
		m_host.release();
	}

	HWND get_wnd() const {
		return appInstance->mainWindow;
	}
	const bool get_is_single_instance(void) const {
		return true;
	}
};

static service_factory_single_t<cui_chronflow> cui_chronflow_instance;