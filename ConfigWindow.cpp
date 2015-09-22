#include "stdafx.h"
#include "base.h"
#include "config.h"

#include "AppInstance.h"
#include "DbAlbumCollection.h"
#include "MyActions.h"
#include "PlaybackTracer.h"
#include "RenderThread.h"
#include "ScriptedCoverPositions.h"

static struct {
   int id;
   cfg_string *var;
} stringVarMap[] = 
{
	// Sources
	{ IDC_SOURCES, &cfgSources },
	{ IDC_FILTER, &cfgFilter },
	{ IDC_GROUP, &cfgGroup },
	{ IDC_SORT, &cfgSort },
	{ IDC_INNER_SORT, &cfgInnerSort },
	{ IDC_IMG_NO_COVER, &cfgImgNoCover },
	{ IDC_IMG_LOADING, &cfgImgLoading },

	// Display
	{ IDC_ALBUM_FORMAT, &cfgAlbumTitle },

	// Behaviour
	{ IDC_TARGET_PL, &cfgTargetPlaylist },
};

static struct {
   int id;
   cfg_string *var;
} textListVarMap[] = 
{
	// Behaviour
	{ IDC_DOUBLE_CLICK, &cfgDoubleClick },
	{ IDC_MIDDLE_CLICK, &cfgMiddleClick },
	{ IDC_ENTER_KEY, &cfgEnterKey },

	// Cover Display
	{ IDC_SAVED_SELECT, &cfgCoverConfigSel },
};


static struct {
	int id;
	cfg_bool *var;
} boolVarMap[] =
{
	// Sources
	{ IDC_SORT_GROUP, &cfgSortGroup },
	{ IDC_PL_SORT_PL, &cfgPlSortPl },

	// Behaviour
	{ IDC_FOLLOW_PLAYBACK, &cfgCoverFollowsPlayback },
	{ IDC_FIND_AS_YOU_TYPE, &cfgFindAsYouType },

	// Display
	{ IDC_ALBUM_TITLE, &cfgShowAlbumTitle },

	// Performance
	{ IDC_MULTI_SAMPLING, &cfgMultisampling },
	{ IDC_SUPER_SAMPLING, &cfgSupersampling },
	{ IDC_TEXTURE_COMPRESSION, &cfgTextureCompression },
	{ IDC_EMPTY_ON_MINIMIZE, &cfgEmptyCacheOnMinimize },
	{ IDC_SHOW_FPS, &cfgShowFps },
};

static struct {
	int checkboxId;
	int itemToDisable;
} disableMap[] =
{
	// Sources
	{ IDC_SORT_GROUP, -IDC_SORT },

	// Behaviour
	{ IDC_FOLLOW_PLAYBACK, IDC_FOLLOW_DELAY },

	// Performance
	{ IDC_SUPER_SAMPLING, IDC_SUPER_SAMPLING_PASSES },
	{ IDC_SUPER_SAMPLING, -IDC_MULTI_SAMPLING },
	{ IDC_MULTI_SAMPLING, IDC_MULTI_SAMPLING_PASSES },
	{ IDC_MULTI_SAMPLING, -IDC_SUPER_SAMPLING },
};


class ConfigTab {
protected:
	UINT id;
	HWND parent;
	char * title;
	bool initializing;
public:
	const int idx;
	HWND hWnd;
	ConfigTab(char * title, UINT id, HWND parent, int& i)
		: idx(i)
	{
		i++;
		this->title = title;
		this->id = id;
		this->parent = parent;
	}
	virtual ~ConfigTab() {
		DestroyWindow(hWnd);
	}
	void createDialog (){
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
			SetWindowLongPtr (hWnd, GWLP_USERDATA, (LONG_PTR)configTab);
			configTab->hWnd = hWnd;
		} else {
			configTab = reinterpret_cast<ConfigTab*>(GetWindowLongPtr(hWnd,GWLP_USERDATA));
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
		n = tabsize(boolVarMap);
		for (int i = 0; i < n; i++){
			uButton_SetCheck(hWnd, boolVarMap[i].id, boolVarMap[i].var->get_value());
		}
		n = tabsize(disableMap);
		for (int i = 0; i < n; i++){
			bool enabled = uButton_GetCheck(hWnd, disableMap[i].checkboxId);
			if (disableMap[i].itemToDisable < 0)
				uEnableWindow(uGetDlgItem(hWnd, -disableMap[i].itemToDisable), !enabled);
			else
				uEnableWindow(uGetDlgItem(hWnd, disableMap[i].itemToDisable), enabled);
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
		n = tabsize(disableMap);
		for (int i = 0; i < n; i++){
			if (disableMap[i].checkboxId == id){
				bool enabled = uButton_GetCheck(hWnd, disableMap[i].checkboxId);
				if (disableMap[i].itemToDisable < 0)
					uEnableWindow(uGetDlgItem(hWnd, -disableMap[i].itemToDisable), !enabled);
				else
					uEnableWindow(uGetDlgItem(hWnd, disableMap[i].itemToDisable), enabled);
			}
		}
	}
	void listSelChanged(UINT id){
		int n = tabsize(textListVarMap);
		for (int i = 0; i < n; i++){
		   if (textListVarMap[i].id == id){
			   int s = uSendDlgItemMessage(hWnd, id, CB_GETCURSEL, 0, 0);
			   if (s != CB_ERR){
				   uComboBox_GetText(uGetDlgItem(hWnd, id), s, *(textListVarMap[i].var));
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

protected:
	void redrawMainWin(){
		FOR_EACH_INSTANCE(redrawMainWin());
	}
};

#define CONFIG_TAB(CLASS, TITLE, IDD) CLASS(HWND parent, int& i) : ConfigTab(TITLE, IDD, parent, i){ createDialog(); }

#if 0
// minimal Tab setup
class SomeTab : public ConfigTab {
public:
	CONFIG_TAB(SomeTab, TITLE, IDD);
	BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		switch (uMsg)
		{
		case WM_INITDIALOG:
			loadConfig();
			break;

		case WM_COMMAND:
			if (HIWORD(wParam) == EN_CHANGE){
				textChanged(LOWORD(wParam));
			} else if (HIWORD(wParam) == BN_CLICKED) {
				buttonClicked(LOWORD(wParam));
			}
			break;
		}
		return FALSE;
	}
};
#endif


class SourcesTab : public ConfigTab {
public:
	CONFIG_TAB(SourcesTab, "Album Source", IDD_SOURCE_TAB);

	BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		switch (uMsg)
		{
		case WM_INITDIALOG:
			loadConfig();
			{
				if (!cfgPlSortPl)
					uButton_SetCheck(hWnd, IDC_PL_SORT_DB, true);
			}
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
						FOR_EACH_INSTANCE(albumCollection->startAsyncReload());
					}
					break;
				case IDC_PL_SORT_DB:
					buttonClicked(IDC_PL_SORT_PL);
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
	CONFIG_TAB(BehaviourTab, "Behaviour", IDD_BEHAVIOUR_TAB);
	BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		switch (uMsg)
		{
		case WM_INITDIALOG:
			loadConfig();
			loadActionList(IDC_DOUBLE_CLICK, cfgDoubleClick);
			loadActionList(IDC_MIDDLE_CLICK, cfgMiddleClick);
			loadActionList(IDC_ENTER_KEY, cfgEnterKey);
			SendDlgItemMessage(hWnd, IDC_FOLLOW_DELAY_SPINNER, UDM_SETRANGE32, 1, 999);
			SetDlgItemInt(hWnd, IDC_FOLLOW_DELAY, cfgCoverFollowDelay, true);
			break;

		case WM_COMMAND:
			if (HIWORD(wParam) == EN_CHANGE){
				textChanged(LOWORD(wParam));
				if (LOWORD(wParam) == IDC_FOLLOW_DELAY){
					cfgCoverFollowDelay = max(1, min(999, int(uGetDlgItemInt(hWnd, IDC_FOLLOW_DELAY, 0, 1))));
					FOR_EACH_INSTANCE(playbackTracer->followSettingsChanged());
				}
			} else if (HIWORD(wParam) == BN_CLICKED) {
				buttonClicked(LOWORD(wParam));
				if (LOWORD(wParam) == IDC_FOLLOW_PLAYBACK){
					FOR_EACH_INSTANCE(playbackTracer->followSettingsChanged());
				}
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
				if (strcmp(menuPath, "Play") == 0){
					continue;
				}
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

class DisplayTab : public ConfigTab {
public:
	CONFIG_TAB(DisplayTab, "Display", IDD_DISPLAY_TAB);

	BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		switch (uMsg)
		{
		case WM_INITDIALOG:
			loadConfig();
			{
				int titlePosH = (int)floor(cfgTitlePosH*100 + 0.5);
				int titlePosV = (int)floor(cfgTitlePosV*100 + 0.5);
				uSendDlgItemMessage(hWnd, IDC_TPOS_H, TBM_SETRANGE, FALSE, MAKELONG(0, 100));
				uSendDlgItemMessage(hWnd, IDC_TPOS_H, TBM_SETTIC, 0, 50);
				uSendDlgItemMessage(hWnd, IDC_TPOS_H, TBM_SETPOS, TRUE, titlePosH);
				uSendDlgItemMessageText(hWnd, IDC_TPOS_H_P, WM_SETTEXT, 0, pfc::string_fixed_t<16>() << titlePosH);

				uSendDlgItemMessage(hWnd, IDC_TPOS_V, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
				uSendDlgItemMessage(hWnd, IDC_TPOS_V, TBM_SETTIC, 0, 50);
				uSendDlgItemMessage(hWnd, IDC_TPOS_V, TBM_SETPOS, TRUE, titlePosV);
				uSendDlgItemMessageText(hWnd, IDC_TPOS_V_P, WM_SETTEXT, 0, pfc::string_fixed_t<16>() << titlePosV);

				uSendDlgItemMessage(hWnd, IDC_FONT_PREV, WM_SETTEXT, 0, (LPARAM)cfgTitleFont.get_value().lfFaceName);

				uSendDlgItemMessage(hWnd, IDC_FRAME_WIDTH_SPIN, UDM_SETRANGE, 0, MAKELONG(short(30),short(0)));
				uSetDlgItemText(hWnd, IDC_FRAME_WIDTH, pfc::string_fixed_t<16>() << cfgHighlightWidth);
			}
			break;
		case WM_HSCROLL:
			{
				int titlePosH = uSendDlgItemMessage(hWnd, IDC_TPOS_H, TBM_GETPOS, 0, 0);
				int titlePosV = uSendDlgItemMessage(hWnd, IDC_TPOS_V, TBM_GETPOS, 0, 0);
				cfgTitlePosH = 0.01 * titlePosH;
				cfgTitlePosV = 0.01 * titlePosV;
				uSendDlgItemMessageText(hWnd, IDC_TPOS_H_P, WM_SETTEXT, 0, pfc::string_fixed_t<16>() << titlePosH);
				uSendDlgItemMessageText(hWnd, IDC_TPOS_V_P, WM_SETTEXT, 0, pfc::string_fixed_t<16>() << titlePosV);

				redrawMainWin();
			}
			break;
		case WM_DRAWITEM:
			{
				DRAWITEMSTRUCT * drawStruct = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
				HBRUSH brush;
				switch (wParam){
				case IDC_TEXTCOLOR_PREV:
					brush = CreateSolidBrush(cfgTitleColor);
					break;

				case IDC_BG_COLOR_PREV:
					brush = CreateSolidBrush(cfgPanelBg);
					break;
				}
				FillRect(drawStruct->hDC, &drawStruct->rcItem, brush);
			}
			break;

		case WM_COMMAND:
			if (HIWORD(wParam) == EN_CHANGE){
				textChanged(LOWORD(wParam));

				switch (LOWORD(wParam))
				{
				case IDC_ALBUM_FORMAT:
					{
						static_api_ptr_t<titleformat_compiler> compiler;
						static_api_ptr_t<metadb> db;
						compiler->compile(cfgAlbumTitleScript, cfgAlbumTitle);
						pfc::string8 preview;
						metadb_handle_ptr aTrack;
						if (db->g_get_random_handle(aTrack)){
							aTrack->format_title(0, preview, cfgAlbumTitleScript, 0);
							uSendDlgItemMessageText(hWnd, IDC_TITLE_PREVIEW, WM_SETTEXT, 0, preview);
						}
						redrawMainWin();
					}
					break;
				case IDC_FRAME_WIDTH:
					{
						pfc::string_fixed_t<16> highlightWidth;
						uGetDlgItemText(hWnd, IDC_FRAME_WIDTH, highlightWidth);
						cfgHighlightWidth = max(0, min(30, atoi(highlightWidth)));
						redrawMainWin();
					}
					break;
				}
			} else if (HIWORD(wParam) == BN_CLICKED) {
				buttonClicked(LOWORD(wParam));
				switch (LOWORD(wParam))
				{
				case IDC_BG_COLOR:
					{
						COLORREF panelBg = cfgPanelBg;
						if (selectColor(panelBg)){
							cfgPanelBg = panelBg;
							InvalidateRect(uGetDlgItem(hWnd, IDC_BG_COLOR_PREV), NULL, TRUE);
						}
					}
					break;
				case IDC_TEXTCOLOR:
					{
						COLORREF titleColor = cfgTitleColor;
						if (selectColor(titleColor)){
							cfgTitleColor = titleColor;
							InvalidateRect(uGetDlgItem(hWnd, IDC_TEXTCOLOR_PREV), NULL, TRUE);
						}
					}
					break;
				case IDC_FONT:
					{
						LOGFONT titleFont = cfgTitleFont;
						if (selectFont(titleFont)){
							cfgTitleFont = titleFont;
							uSendDlgItemMessage(hWnd, IDC_FONT_PREV, WM_SETTEXT, 0, (LPARAM)cfgTitleFont.get_value().lfFaceName);
							auto msg = make_shared<RTTextFormatChangedMessage>();
							FOR_EACH_INSTANCE(renderer->send(msg));
						}
					}
				}
				redrawMainWin();
			}
			break;
		}
		return FALSE;
	}
private:
	bool selectColor(COLORREF& color){
		static DWORD costumColors[16] = {0};
		COLORREF tColor = color;
		if (uChooseColor(&tColor, hWnd, costumColors)){
			color = tColor;
			return true;
		} else {
			return false;
		}
	}
	bool selectFont(LOGFONT& font){
		LOGFONT tFont = font;

		CHOOSEFONT cf = {0};
		cf.lStructSize = sizeof(cf);
		cf.hwndOwner = hWnd;
		cf.lpLogFont = &tFont;
		cf.Flags = CF_TTONLY | CF_SCREENFONTS | CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT;
		if (ChooseFont(&cf)){
			font = tFont;
			return true;
		} else {
			return false;
		}
	}
};

class ConfigNameDialog : private dialog_helper::dialog_modal {
	BOOL on_message(UINT uMsg, WPARAM wParam, LPARAM lParam){
		switch (uMsg){
			case WM_INITDIALOG:
				if (value){
					uSetDlgItemText(get_wnd(), IDC_CONFIG_NAME, value);
				}
				SetFocus(uGetDlgItem(get_wnd(), IDC_CONFIG_NAME));
				return 0;
			case WM_COMMAND:
				if (HIWORD(wParam) == BN_CLICKED){
					if (LOWORD(wParam) == IDOK)
						end_dialog(1);
					else if (LOWORD(wParam) == IDCANCEL)
						end_dialog(0);
				} else if (HIWORD(wParam) == EN_CHANGE){
					uGetDlgItemText(get_wnd(), IDC_CONFIG_NAME, value);
				}
				return 0;
		}
		return 0;
	}
public:
	int query(HWND parent, const char * defValue = ""){
		value = defValue;
		return run (IDD_CONFIG_NAME, parent);
	}
	pfc::string8 value;
};

class CoverTab : public ConfigTab {
	HFONT editBoxFont;
	WNDPROC origEditboxProc;

public:
	CONFIG_TAB(CoverTab, "Cover Display", IDD_COVER_DISPLAY_TAB);

	BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		switch (uMsg)
		{
		case WM_INITDIALOG:
			editBoxFont = CreateFont(-12, 0, 0, 0, 0, FALSE, 0, 0, 0, 0, 0, 0, 0, L"Courier New");
			loadConfig();
			loadConfigList();
			configSelectionChanged();
			setUpEditBox();
			return TRUE;
		
		case WM_DESTROY:
			DeleteObject(editBoxFont);

		
		case WM_COMMAND:
			if (HIWORD(wParam) == EN_CHANGE){
				textChanged(LOWORD(wParam));
				if (LOWORD(wParam) == IDC_DISPLAY_CONFIG){
					CoverConfig* config = cfgCoverConfigs.getPtrByName(cfgCoverConfigSel);
					uGetDlgItemText(hWnd, IDC_DISPLAY_CONFIG, config->script);
					uSetDlgItemText(hWnd, IDC_COMPILE_STATUS, "");
				}
			} else if (HIWORD(wParam) == BN_CLICKED) {
				buttonClicked(LOWORD(wParam));
				if (LOWORD(wParam) == IDC_SAVED_ADD)
					addConfig();
				else if (LOWORD(wParam) == IDC_SAVED_RENAME)
					renameConfig();
				else if (LOWORD(wParam) == IDC_SAVED_REMOVE)
					removeConfig();
				else if (LOWORD(wParam) == IDC_COMPILE)
					compileConfig();
			} else if (HIWORD(wParam) == CBN_SELCHANGE){
				listSelChanged(LOWORD(wParam));
				if (LOWORD(wParam) == IDC_SAVED_SELECT){
					configSelectionChanged();
				}
			}
			break;
		}
		return FALSE;
	}
	void compileConfig(){
		pfc::string8 message;
		pfc::string8 script;
		uGetDlgItemText(hWnd, IDC_DISPLAY_CONFIG, script);

		try {
			ScriptedCoverPositions testCovPos;
			if (!testCovPos.setScript(script, message)){
				uSetDlgItemText(hWnd, IDC_COMPILE_STATUS, message);
				return;
			} else {
				uSetDlgItemText(hWnd, IDC_COMPILE_STATUS, "Compilation successfull");
			}
		}
		catch (pfc::exception& e){
			message = e.what();
			uSetDlgItemText(hWnd, IDC_COMPILE_STATUS, message);
			return;
		}

		auto msg = make_shared<RTChangeCPScriptMessage>(script);
		FOR_EACH_INSTANCE(renderer->send(msg));
	}
	void setUpEditBox(){
		int tabstops[1] = {14};
		SendDlgItemMessage(hWnd, IDC_DISPLAY_CONFIG, EM_SETTABSTOPS, 1, (LPARAM)tabstops);
		SendDlgItemMessage(hWnd, IDC_DISPLAY_CONFIG, WM_SETFONT, (WPARAM)editBoxFont, TRUE);
		origEditboxProc = (WNDPROC) SetWindowLong(GetDlgItem(hWnd, IDC_DISPLAY_CONFIG), GWL_WNDPROC, (LONG)editboxProxy);
		SetProp(GetDlgItem(hWnd, IDC_DISPLAY_CONFIG), L"tab", (HANDLE)this);
	}
	static BOOL CALLBACK editboxProxy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		CoverTab* coverTab = reinterpret_cast<CoverTab*>(GetProp(hWnd, L"tab"));
		return coverTab->editboxProc(hWnd, uMsg, wParam, lParam);
	}
	BOOL editboxProc(HWND eWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		if (uMsg == WM_KEYDOWN && wParam == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)){
			SendMessage(eWnd, EM_SETSEL, 0, -1);
			return false;
		}

		if (GetWindowLong(eWnd, GWL_STYLE) & ES_READONLY)
			return CallWindowProc(origEditboxProc, eWnd, uMsg, wParam, lParam);

		if (uMsg == WM_KEYDOWN){
			if (wParam == VK_TAB){
				DWORD selFirst;
				DWORD selLast;
				SendMessage(eWnd, EM_GETSEL, (WPARAM)&selFirst, (LPARAM)&selLast);
				if (selFirst == selLast){
					uSendMessageText(eWnd, EM_REPLACESEL, TRUE, "\t");
				} else {
					bool intend = !(GetKeyState(VK_SHIFT) & 0x8000);
					pfc::string8 boxText;
					uGetWindowText(eWnd, boxText);
					PFC_ASSERT(selLast <= boxText.length());
					const char * pStart = boxText.get_ptr();
					const char * selEnd = pStart + selLast;
					const char * p = pStart + selFirst;
					while ((p > pStart) && (p[-1] != '\n'))
						p--;
					unsigned int repFirst = p - pStart;
					if (intend){
						selFirst++;
					} else {
						if (*p == '\t') {
							selFirst--;
						} else if ((p+2 < selEnd) && p[0] == ' ' && p[1] == ' ' && p[2] == ' ') {
							selFirst -= 3;
						}
						if (selFirst < repFirst)
							selFirst = repFirst;
					}
					pfc::string8 outText;
					outText.prealloc(selEnd - p);
					const char * lStart = p;
					p++;
					while (p <= selEnd){
						while(p[-1] != '\n' && p < selEnd)
							p++;
						if (intend){
							outText.add_char('\t');
						} else {
							if (*lStart == '\t') {
								lStart++;
							} else if ((lStart+3 < p) && lStart[0] == ' ' && lStart[1] == ' ' && lStart[2] == ' ') {
								lStart += 3;
							}
						}
						outText.add_string(lStart, p - lStart);
						lStart = p;
						p++;
					}
					SendMessage(eWnd, EM_SETSEL, repFirst, selLast);
					uSendMessageText(eWnd, EM_REPLACESEL, TRUE, outText);
					SendMessage(eWnd, EM_SETSEL, selFirst, repFirst+outText.length());
				}
				return false;
			} else if (wParam == VK_RETURN) {
				DWORD selFirst;
				DWORD selLast;
				SendMessage(eWnd, EM_GETSEL, (WPARAM)&selFirst, (LPARAM)&selLast);
				pfc::string8 boxText;
				uGetWindowText(eWnd, boxText);
				const char * pStart = boxText.get_ptr();
				const char * selEnd = pStart + selLast;
				const char * p = pStart + selFirst;
				int tabCount = 0;
				if (p > pStart && p[-1] == '{')
					tabCount++;
				while ((p > pStart) && (p[-1] != '\n'))
					p--;
				while (*p == '\t' || ((p+2 < selEnd) && *p == ' ' && *(++p) == ' ' && *(++p) == ' ')){
					p++;
					tabCount++;
				}
				pfc::string8 intend("\r\n");
				intend.add_chars('\t', tabCount);
				uSendMessageText(eWnd, EM_REPLACESEL, TRUE, intend);
				return false;
			}
		} else if (uMsg == WM_CHAR) {
			if (wParam == 1){
				return false;
			} else if (wParam == VK_RETURN) {
				return false;
			} else if (wParam == '}'){
				wchar_t line[3];
				line[0] = 3;
				DWORD selFirst;
				SendMessage(eWnd, EM_GETSEL, (WPARAM)&selFirst, 0);
				int lineNumber = SendMessage(eWnd, EM_LINEFROMCHAR, selFirst, 0);
				int lineLength = SendMessage(eWnd, EM_LINELENGTH, selFirst, 0);
				int lineIdx = SendMessage(eWnd, EM_LINEINDEX, lineNumber, 0);
				SendMessage(eWnd, EM_GETLINE, lineNumber, (LPARAM)&line);
				uSendMessageText(eWnd, EM_REPLACESEL, TRUE, "}");
				int deleteChars = 0;
				if (lineLength > 0 && line[0] == '\t')
					deleteChars = 1;
				else if (lineLength > 2 && line[0] == ' ' && line[1] == ' ' && line[2] == ' ')
					deleteChars = 3;
				if (deleteChars > 0){
					SendMessage(eWnd, EM_SETSEL, (WPARAM)lineIdx, (LPARAM)(lineIdx + deleteChars));
					uSendMessageText(eWnd, EM_REPLACESEL, TRUE, "");
					SendMessage(eWnd, EM_SETSEL, selFirst-deleteChars+1, selFirst-deleteChars+1);
				}
				return false;
			}
		}
		return CallWindowProc(origEditboxProc, eWnd, uMsg, wParam, lParam);
	}
	void removeConfig(){
		if (cfgCoverConfigs.get_count() > 1){
			pfc::string8 title;
			title << "Delete Config \"" << cfgCoverConfigs.getPtrByName(cfgCoverConfigSel)->name <<"\"";
			if (IDYES == MessageBoxA(hWnd, "Are you sure?", title, MB_APPLMODAL|MB_YESNO|MB_ICONQUESTION)){
				cfgCoverConfigs.sortByName();
				t_size configIdx = 0;
				for (t_size i=0; i < cfgCoverConfigs.get_count(); i++){
					if (!stricmp_utf8(cfgCoverConfigs[i].name, cfgCoverConfigSel)){
						configIdx = i;
					}
				}
				cfgCoverConfigs.remove_by_idx(configIdx);
				if (configIdx == cfgCoverConfigs.get_count()){
					configIdx--;
				}
				cfgCoverConfigSel = cfgCoverConfigs[configIdx].name;
				loadConfigList();
				configSelectionChanged();
			}
		}
	}
	void addConfig(){
		ConfigNameDialog dialog;
		if (dialog.query(hWnd)){
			dialog.value.skip_trailing_char(' ');
			if (dialog.value.get_length()){
				if (!cfgCoverConfigs.getPtrByName(dialog.value)){
					CoverConfig config;
					config.name = dialog.value;
					bool useClipboard = false;
					if (uGetClipboardString(config.script)){
						bool allFound = true;
						pfc::stringcvt::string_wide_from_utf8 clipboard_w(config.script);
						for (int i=0; i < CPScriptFuncInfos::funcCount; i++){
							if (CPScriptFuncInfos::neededFunctions[i]){
								if (wcsstr(clipboard_w, CPScriptFuncInfos::knownFunctions[i]) == 0){
									allFound = false;
									break;
								}
							}
						}
						if (allFound)
							useClipboard = true;
					}
					if (!useClipboard)
						config.script = COVER_CONFIG_DEF_CONTENT;
					config.buildIn = false;
					cfgCoverConfigs.add_item(config);
					cfgCoverConfigSel = dialog.value;
					loadConfigList();
					configSelectionChanged();
				}
			}
		}
	}
	void renameConfig(){
		CoverConfig* config = cfgCoverConfigs.getPtrByName(cfgCoverConfigSel);
		if (config){
			ConfigNameDialog dialog;
			if (dialog.query(hWnd, config->name)){
				dialog.value.skip_trailing_char(' ');
				if (dialog.value.get_length()){
					CoverConfig* nameCollision = cfgCoverConfigs.getPtrByName(dialog.value);
					if (!nameCollision || nameCollision == config){
						config->name = dialog.value;
						cfgCoverConfigSel = dialog.value;
						loadConfigList();
						configSelectionChanged();
					}
				}
			}
		}
	}
	void loadConfigList(){
		uSendDlgItemMessage(hWnd, IDC_SAVED_SELECT, CB_RESETCONTENT, 0, 0);

		t_size n, m = cfgCoverConfigs.get_count();
		for (n=0; n < m; n++){
			uSendDlgItemMessageText(hWnd, IDC_SAVED_SELECT, CB_ADDSTRING, 0, cfgCoverConfigs.get_item_ref(n).name);
		}

		uSendDlgItemMessageText(hWnd, IDC_SAVED_SELECT, CB_SELECTSTRING, -1, cfgCoverConfigSel);
	}
	void configSelectionChanged(){
		const CoverConfig* config = cfgCoverConfigs.getPtrByName(cfgCoverConfigSel);
		if (config){
			uSetDlgItemText(hWnd, IDC_DISPLAY_CONFIG, config->script);
			uSendDlgItemMessage(hWnd, IDC_DISPLAY_CONFIG, EM_SETREADONLY, (int)config->buildIn, 0);
			//uEnableWindow(uGetDlgItem(hWnd, IDC_DISPLAY_CONFIG), !config->buildIn);
			uEnableWindow(uGetDlgItem(hWnd, IDC_SAVED_REMOVE), !config->buildIn);
			uEnableWindow(uGetDlgItem(hWnd, IDC_SAVED_RENAME), !config->buildIn);
		}
		uSetDlgItemText(hWnd, IDC_COMPILE_STATUS, "");
	}
};

namespace {
	struct ListMap {
		int val;
		const char * text;
	};
	static ListMap multisamplingMap[] =
	{
		{ 2, "  2"},
		{ 4, "  4"},
		{ 8, "  8"},
		{16,  "16"},
	};
	static ListMap supersamplingMap[] =
	{
		{ 2, "  2"},
		{ 3, "  3"},
		{ 4, "  4"},
		{ 5, "  5"},
		{ 6, "  6"},
		{ 8, "  8"},
		{16,  "16"},
	};
	static ListMap loaderPrioMap[] = 
	{
		{THREAD_PRIORITY_BELOW_NORMAL, "Below Normal"},
		{THREAD_PRIORITY_IDLE, "Idle"},
	};
};
static struct {
	int id;
	cfg_int* var;
	ListMap* map;
	t_size mapSize;
} mappedListVarMap[] =
{
	{IDC_MULTI_SAMPLING_PASSES, &cfgMultisamplingPasses, multisamplingMap, tabsize(multisamplingMap)},
	{IDC_SUPER_SAMPLING_PASSES, &cfgSupersamplingPasses, supersamplingMap, tabsize(supersamplingMap)},
	{IDC_TEXLOADER_PRIO, &cfgTexLoaderPrio, loaderPrioMap, tabsize(loaderPrioMap)},
};





class PerformanceTab : public ConfigTab {
public:
	CONFIG_TAB(PerformanceTab, "Performance", IDD_PERF_TAB);
	BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		switch (uMsg)
		{
		case WM_INITDIALOG:
			loadConfig();
			fillComboBoxes();

			SendDlgItemMessage(hWnd, IDC_CACHE_SIZE_SPIN, UDM_SETRANGE32, 2, 999);
			SetDlgItemInt(hWnd, IDC_CACHE_SIZE, cfgTextureCacheSize, true);

			SendDlgItemMessage(hWnd, IDC_TEXTURE_SIZE_SPIN, UDM_SETRANGE32, 4, 2024);
			SetDlgItemInt(hWnd, IDC_TEXTURE_SIZE, cfgMaxTextureSize, true);
			
			switch (cfgVSyncMode){
				case VSYNC_SLEEP_ONLY:
					uButton_SetCheck(hWnd, IDC_VSYNC_OFF, true); break;
				case VSYNC_ONLY:
					uButton_SetCheck(hWnd, IDC_VSYNC_ONLY, true); break;
				case VSYNC_AND_SLEEP:
					uButton_SetCheck(hWnd, IDC_VSYNC_SLEEP, true); break;
			}
			break;

		case WM_COMMAND:
			if (HIWORD(wParam) == EN_CHANGE){
				textChanged(LOWORD(wParam));

				if (LOWORD(wParam) == IDC_CACHE_SIZE){
					cfgTextureCacheSize = max(2, min(999, int(uGetDlgItemInt(hWnd, IDC_CACHE_SIZE, 0, 1))));
				} else if (LOWORD(wParam) == IDC_TEXTURE_SIZE){
					cfgMaxTextureSize = max(4, min(2024, int(uGetDlgItemInt(hWnd, IDC_TEXTURE_SIZE, 0, 1))));
				}
			} else if (HIWORD(wParam) == BN_CLICKED) {
				buttonClicked(LOWORD(wParam));
				switch (LOWORD(wParam)){
					case IDC_VSYNC_OFF:
					case IDC_VSYNC_ONLY:
					case IDC_VSYNC_SLEEP:
						if (uButton_GetCheck(hWnd, IDC_VSYNC_OFF)) cfgVSyncMode = VSYNC_SLEEP_ONLY;
						else if (uButton_GetCheck(hWnd, IDC_VSYNC_ONLY)) cfgVSyncMode = VSYNC_ONLY;
						else if (uButton_GetCheck(hWnd, IDC_VSYNC_SLEEP)) cfgVSyncMode = VSYNC_AND_SLEEP;
				}
				redrawMainWin();
			} else if (HIWORD(wParam) == CBN_SELCHANGE){
				comboBoxChanged(LOWORD(wParam));
				redrawMainWin();
			}
			break;
			
		}
		return FALSE;
	}
	void fillComboBoxes(){
		for (int i=0; i < tabsize(mappedListVarMap); i++){
			uSendDlgItemMessage(hWnd, mappedListVarMap[i].id, CB_RESETCONTENT, 0, 0);
			for (t_size n=0; n < mappedListVarMap[i].mapSize; n++){
				uSendDlgItemMessageText(hWnd, mappedListVarMap[i].id, CB_ADDSTRING, 0, mappedListVarMap[i].map[n].text);
				if (mappedListVarMap[i].map[n].val == *(mappedListVarMap[i].var))
					uSendDlgItemMessageText(hWnd, mappedListVarMap[i].id, CB_SELECTSTRING, -1, mappedListVarMap[i].map[n].text);
			}
		}
	}
	void comboBoxChanged(int comboBox){
		pfc::string8 selected;
		int s = uSendDlgItemMessage(hWnd, comboBox, CB_GETCURSEL, 0, 0);
		if (s != CB_ERR){
			uComboBox_GetText(uGetDlgItem(hWnd, comboBox), s, selected);
		} else {
			return;
		}
		for (int i=0; i < tabsize(mappedListVarMap); i++){
			if (comboBox == mappedListVarMap[i].id){
				for (t_size n=0; n < mappedListVarMap[i].mapSize; n++){
					if (0 == strcmp(selected, mappedListVarMap[i].map[n].text)){
						*(mappedListVarMap[i].var) = mappedListVarMap[i].map[n].val;
						break;
					}
				}
			}
		}
	}
};


class ConfigWindow :
	public preferences_page
{
private:
	pfc::list_t<ConfigTab*> tabs;
	t_size currentTab;
public:
	ConfigWindow()
		: currentTab(~0)
	{
	}

	BOOL CALLBACK dialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		switch (uMsg)
		{
		case WM_INITDIALOG:
			{
				int j = 0;
				tabs.add_item(new BehaviourTab(hWnd, j));
				tabs.add_item(new SourcesTab(hWnd, j));
				tabs.add_item(new DisplayTab(hWnd, j));
				tabs.add_item(new CoverTab(hWnd, j));
				tabs.add_item(new PerformanceTab(hWnd, j));

				HWND hWndTab = uGetDlgItem(hWnd, IDC_TABS);

				RECT rcTab;
				GetChildWindowRect(hWnd, IDC_TABS, &rcTab);
				uSendMessage(hWndTab, TCM_ADJUSTRECT, FALSE, (LPARAM)&rcTab);

				for (t_size i=0; i < tabs.get_count(); i++){
					tabs[i]->setPos(rcTab);
				}
				currentTab = sessionSelectedConfigTab;
				uSendMessage(hWndTab, TCM_SETCURSEL, tabs[currentTab]->idx, 0);
				tabs[currentTab]->show();
			}
			break;

		case WM_NCDESTROY:
			{
				sessionSelectedConfigTab = currentTab;
				for (t_size i=0; i < tabs.get_count(); i++){
					delete tabs[i];
				}
				tabs.remove_all();
			}
			break;

		case WM_NOTIFY:
			if (((LPNMHDR)lParam)->idFrom == IDC_TABS){
				if (((LPNMHDR)lParam)->code == TCN_SELCHANGE){
					if (currentTab < tabs.get_count())
						tabs[currentTab]->hide();
					currentTab = ~0;
					UINT32 currentIdx = SendDlgItemMessage(hWnd, IDC_TABS, TCM_GETCURSEL, 0, 0);
					for (t_size i=0; i < tabs.get_count(); i++){
						if(currentIdx == tabs[i]->idx){
							currentTab = i;
							break;
						}
					}
					if (currentTab != ~0)
						tabs[currentTab]->show();
				}
			}
			break;
		}
		return FALSE;
	}

	static BOOL CALLBACK dialogProxy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		ConfigWindow* configWindow = 0;
		if (uMsg == WM_INITDIALOG){
			configWindow = (ConfigWindow*)lParam;
			SetWindowLongPtr (hWnd, GWLP_USERDATA, (LONG_PTR)configWindow);
		} else {
			configWindow = reinterpret_cast<ConfigWindow*>(GetWindowLongPtr(hWnd,GWLP_USERDATA));
		}
		return configWindow->dialogProc(hWnd, uMsg, wParam, lParam);
	}

	HWND create(HWND parent){
		return uCreateDialog(IDD_CONFIG_TABS, parent, dialogProxy, (LPARAM)this);
	}

	const char * get_name(){
		return "Chronflow";
	}

	GUID get_guid(){
		return guid_configWindow;
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