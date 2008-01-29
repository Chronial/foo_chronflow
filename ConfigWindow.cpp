#include "chronflow.h"

extern cfg_string cfgFilter;
extern cfg_string cfgSources;
extern cfg_string cfgGroup;
extern cfg_string cfgSort;
extern cfg_string cfgInnerSort;

extern cfg_string cfgImgLoading;
extern cfg_string cfgImgNoCover;

extern cfg_bool cfgSortGroup;
extern cfg_bool cfgPlSortPl;


extern cfg_string cfgDoubleClick;
extern cfg_string cfgMiddleClick;
extern cfg_string cfgEnterKey;

static struct {
   int id;
   cfg_string *var;
} stringVarMap[] = 
{
	{ IDC_SOURCES, &cfgSources },
	{ IDC_FILTER, &cfgFilter },
	{ IDC_GROUP, &cfgGroup },
	{ IDC_SORT, &cfgSort },
	{ IDC_INNER_SORT, &cfgInnerSort },
	{ IDC_IMG_NO_COVER, &cfgImgNoCover },
	{ IDC_IMG_LOADING, &cfgImgLoading },
};

static struct {
   int id;
   cfg_string *var;
} listVarMap[] = 
{
	{ IDC_DOUBLE_CLICK, &cfgDoubleClick },
	{ IDC_MIDDLE_CLICK, &cfgMiddleClick },
	{ IDC_ENTER_KEY, &cfgEnterKey },
};


static struct {
	int id;
	cfg_bool *var;
} boolVarMap[] =
{
	{ IDC_SORT_GROUP, &cfgSortGroup },
	{ IDC_PL_SORT_PL, &cfgPlSortPl },
};

class ConfigTab {
protected:
	UINT id;
	HWND parent;
	char * title;
	bool initializing;
public:
	HWND hWnd;
	ConfigTab(char * title, UINT id, HWND parent){
		this->title = title;
		this->id = id;
		this->parent = parent;
	}
	void init (int idx){
		HWND hWndTab = uGetDlgItem(parent, IDC_TABS);
		uTCITEM tabItem = {0};
		tabItem.mask = TCIF_TEXT;
		tabItem.pszText = title;
		uTabCtrl_InsertItem(hWndTab, idx, &tabItem);

		hWnd = uCreateDialog(id, parent, dialogProxy, (LPARAM)this);
	}
	virtual BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
	static BOOL CALLBACK dialogProxy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		ConfigTab* configTab = 0;
		if (uMsg == WM_INITDIALOG){
			configTab = (ConfigTab*)lParam;
			uSetWindowLong(hWnd, GWL_USERDATA, (LPARAM)configTab);
			configTab->hWnd = hWnd;
		} else {
			configTab = reinterpret_cast<ConfigTab*>(uGetWindowLong(hWnd,GWL_USERDATA));
		}
		if (configTab == 0)
			return FALSE;
		return configTab->dialogProc(hWnd, uMsg, wParam, lParam);
	}
	void show(){
		ShowWindow(hWnd, SW_SHOW);
	}
	void hide(){
		ShowWindow(hWnd, SW_HIDE);
	}
	void loadConfig(){
		initializing = true;
		int n = tabsize(stringVarMap);
		for (int i = 0; i < n; i++){
			uSetDlgItemText(hWnd, stringVarMap[i].id, stringVarMap[i].var->get_ptr());
		}
		int m = tabsize(boolVarMap);
		for (int i = 0; i < m; i++){
			uButton_SetCheck(hWnd, boolVarMap[i].id, boolVarMap[i].var->get_value());
		}
		initializing = false;
	}
	void textChanged(UINT id){
		if (initializing)
			return;
		int n = tabsize(stringVarMap);
		for (int i = 0; i < n; i++){
			if (stringVarMap[i].id == id){
				pfc::string8 buf;
				uGetDlgItemText(hWnd, stringVarMap[i].id, buf);
				(*stringVarMap[i].var) = buf;
				break;
			}
		}
	}
	void buttonClicked(UINT id){
		int n = tabsize(boolVarMap);
		for (int i = 0; i < n; i++){
			if (boolVarMap[i].id == id){
				(*boolVarMap[i].var) = uButton_GetCheck(hWnd, id);
				break;
			}
		}
	}
	void listSelChanged(UINT id){
		int n = tabsize(listVarMap);
		for (int i = 0; i < n; i++){
		   if (listVarMap[i].id == id){
			   int s = uSendDlgItemMessage(hWnd, id, CB_GETCURSEL, 0, 0);
			   if (s != CB_ERR){
				   wchar_t tmpBuffer[2048];
				   uSendDlgItemMessage(hWnd, id, CB_GETLBTEXT, s, (LPARAM)tmpBuffer);
				   (*listVarMap[i].var).set_string(pfc::stringcvt::string_utf8_from_wide(tmpBuffer));
			   }
			   break;
		   }
		}
	}
	void setPos(RECT fitInto){
		SetWindowPos(hWnd, NULL,
			fitInto.left, fitInto.top,
			fitInto.right - fitInto.left, fitInto.bottom - fitInto.top,
			SWP_NOZORDER | SWP_NOACTIVATE);
	}
};

class SourcesTab : public ConfigTab {
public:
	static const int idx = 1;

	SourcesTab(HWND parent) : ConfigTab("Album Source", IDD_SOURCE_TAB, parent){
		init(idx);
	}
	BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		switch (uMsg)
		{
		case WM_INITDIALOG:
			loadConfig();
			if (!cfgPlSortPl)
				uButton_SetCheck(hWnd, IDC_PL_SORT_DB, true);
			uEnableWindow(uGetDlgItem(hWnd, IDC_SORT), !cfgSortGroup);
			break;

		case WM_COMMAND:
			if (HIWORD(wParam) == EN_CHANGE){
				textChanged(LOWORD(wParam));
			} else if (HIWORD(wParam) == BN_CLICKED) {
				buttonClicked(LOWORD(wParam));
				switch (LOWORD(wParam))
				{
				case IDC_BTN_REFRESH:
					{
						HWND mainWin = gGetMainWindow();
						if (mainWin) {
							gCollection->reloadAsynchStart(mainWin, true);
						}
					}
					break;
				case IDC_PL_SORT_DB:
					buttonClicked(IDC_PL_SORT_PL);
					break;
				case IDC_SORT_GROUP:
					uEnableWindow(uGetDlgItem(hWnd, IDC_SORT), !cfgSortGroup);
					break;
				case IDC_IMG_NO_COVER_BROWSE:
					if(browseForImage(cfgImgNoCover, cfgImgNoCover))
						uSetDlgItemText(hWnd, IDC_IMG_NO_COVER, cfgImgNoCover);
					break;
				case IDC_IMG_LOADING_BROWSE:
					if(browseForImage(cfgImgLoading, cfgImgLoading))
						uSetDlgItemText(hWnd, IDC_IMG_LOADING, cfgImgLoading);
					break;
				}
			}
			break;
		}
		return FALSE;
	}
	bool browseForImage(const char * oldImg, pfc::string_base &out){
		OPENFILENAME ofn;
		wchar_t fileName[1024];
		pfc::stringcvt::convert_utf8_to_wide(fileName, 1024, oldImg, ~0);
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = hWnd;
		ofn.lpstrFile = fileName;
		ofn.nMaxFile = 1024;
		ofn.lpstrFilter = L"Image File\0*.jpg;*.jpeg;*.png;*.gif;*.bmp;*.tiff\0";
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST;
		if (GetOpenFileName(&ofn)){
			out = pfc::stringcvt::string_utf8_from_wide(fileName).get_ptr();
			return true;
		} else {
			return false;
		}
	}
};

class BehaviourTab : public ConfigTab {
public:
	static const int idx = 0;

	BehaviourTab (HWND parent) : ConfigTab("Behaviour", IDD_BEHAVIOUR_TAB, parent){
		init(idx);
	}
	BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		switch (uMsg)
		{
		case WM_INITDIALOG:
			loadConfig();
			loadActionList(IDC_DOUBLE_CLICK, cfgDoubleClick);
			loadActionList(IDC_MIDDLE_CLICK, cfgMiddleClick);
			loadActionList(IDC_ENTER_KEY, cfgEnterKey);
			break;

		case WM_COMMAND:
			if (HIWORD(wParam) == EN_CHANGE){
				textChanged(LOWORD(wParam));
			} else if (HIWORD(wParam) == BN_CLICKED) {
				buttonClicked(LOWORD(wParam));
			} else if (HIWORD(wParam) == CBN_SELCHANGE){
				listSelChanged(LOWORD(wParam));
			}
			break;
		}
		return FALSE;
	}
	void loadActionList(UINT id, const char * selectedItem){
		uSendDlgItemMessage(hWnd, id, CB_RESETCONTENT, 0, 0);
		uSendDlgItemMessageText( hWnd, id, CB_ADDSTRING, 0, "" );

		service_enum_t<contextmenu_item> menus;
		service_ptr_t<contextmenu_item> menu;
		while (menus.next(menu)){
			pfc::string8_fastalloc menuPath;
			pfc::string8_fastalloc menuName;
			for (unsigned int i = 0; i < menu->get_num_items(); i++){
				menu->get_item_default_path(i, menuPath);
				menu->get_item_name(i, menuName);
				if (!menuPath.is_empty())
					menuPath.add_string("/");
				menuPath.add_string(menuName);
				uSendDlgItemMessageText(hWnd, id, CB_ADDSTRING, 0, menuPath);
			}
		}
		for (int i = (tabsize(g_customActions)-1); i >= 0; i--){
			uSendDlgItemMessageText(hWnd, id, CB_INSERTSTRING, 0, g_customActions[i]->actionName);
		}
		uSendDlgItemMessageText(hWnd, id, CB_SELECTSTRING, -1, selectedItem);
	}
};

class ConfigWindow :
	public preferences_page
{
private:
	SourcesTab* sourcesTab;
	BehaviourTab* behaviourTab;
	ConfigTab* currentTab;
public:
	ConfigWindow(){
		sourcesTab = 0;
		currentTab = 0;
	}
	BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		switch (uMsg)
		{
		case WM_INITDIALOG:
			{
				sourcesTab = new SourcesTab(hWnd);
				behaviourTab = new BehaviourTab(hWnd);

				HWND hWndTab = uGetDlgItem(hWnd, IDC_TABS);

				RECT rcTab;
				GetChildRect(hWnd, IDC_TABS, &rcTab);
				uSendMessage(hWndTab, TCM_ADJUSTRECT, FALSE, (LPARAM)&rcTab);

				sourcesTab->setPos(rcTab);
				behaviourTab->setPos(rcTab);

				uSendMessage(hWndTab, TCM_SETCURSEL, BehaviourTab::idx, 0);
				currentTab = behaviourTab;
				currentTab->show();
			}
			break;

		case WM_NCDESTROY:
			{
				currentTab = 0;
				delete sourcesTab;
			}
			break;

		case WM_NOTIFY:
			if (((LPNMHDR)lParam)->idFrom == IDC_TABS){
				if (((LPNMHDR)lParam)->code == TCN_SELCHANGE){
					if (currentTab != 0)
						currentTab->hide();
					currentTab = 0;
					UINT32 currentIdx = SendDlgItemMessage(hWnd, IDC_TABS, TCM_GETCURSEL, 0, 0);
					if (currentIdx == SourcesTab::idx){
						currentTab = sourcesTab;
					} else if (currentIdx == BehaviourTab::idx){
						currentTab = behaviourTab;
					}
					if (currentTab != 0)
						currentTab->show();
				}
			}
			break;
		}
		return FALSE;
	}

	static BOOL CALLBACK dialogProxy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		static ConfigWindow* configWindow;
		if (uMsg == WM_INITDIALOG)
			configWindow = (ConfigWindow*)lParam;
		return configWindow->dialogProc(hWnd, uMsg, wParam, lParam);
	}

	HWND create(HWND parent){
		return uCreateDialog(IDD_CONFIG_TABS, parent, dialogProxy, (LPARAM)this);
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