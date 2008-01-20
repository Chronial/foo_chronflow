#include "chronflow.h"

extern cfg_string cfgFilter;
extern cfg_string cfgSources;

struct {
   int id;
   cfg_string *var;
} stringVarMap[] = 
{
	{ IDC_SOURCES, &cfgSources },
	{ IDC_FILTER, &cfgFilter },
};

class ConfigWindow :
	public preferences_page
{
public:
	BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		switch (uMsg)
		{
		case WM_INITDIALOG:
			{
				int n = tabsize(stringVarMap);
				for (int i = 0; i < n; i++){
					uSetDlgItemText(hWnd, stringVarMap[i].id, stringVarMap[i].var->get_ptr());
				}
			}
			break;

		case WM_COMMAND:
			if (HIWORD(wParam) == EN_CHANGE){
				int n = tabsize(stringVarMap);
				for (int i = 0; i < n; i++){
					if (stringVarMap[i].id == LOWORD(wParam)){
						pfc::string8 buf;
						uGetDlgItemText(hWnd, stringVarMap[i].id, buf);
						(*stringVarMap[i].var) = buf;
						break;
					}
				}
			} else if (HIWORD(wParam) == BN_CLICKED) {
				switch (LOWORD(wParam))
				{
				case IDC_BTN_REFRESH:
					break;
				}
			}
			break;
		}
		return false;
	}

	static BOOL CALLBACK dialogProxy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		static ConfigWindow* configWindow;
		if (uMsg == WM_INITDIALOG)
			configWindow = (ConfigWindow*)lParam;

		return configWindow->dialogProc(hWnd, uMsg, wParam, lParam);
	}

	HWND create(HWND parent){
		return uCreateDialog(IDD_CONFIG, parent, dialogProxy, (LPARAM)this);
	}

	const char * get_name(){
		return "Chronflow";
	}

	GUID get_guid(){
		// {37835416-4578-4aaa-A229-E09AB9E2CB9C}
		static const GUID guid_preferences = 
		{ 0x37835416, 0x4578, 0x4aaa, { 0xa2, 0x29, 0xe0, 0x9a, 0xb9, 0xe2, 0xcb, 0x9c } };

		return guid_preferences;
	}

	GUID get_parent_guid(){
		return preferences_page::guid_display;
	}

	bool reset_query(){
		return false;
	}
	void reset(){}
};

static service_factory_single_t<ConfigWindow> x_configWindow;