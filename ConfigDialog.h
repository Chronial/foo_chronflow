#pragma once
#include <helpers/atl-misc.h>
#include <helpers/DarkMode.h>
#include <libPPUI/win32_utility.h>
#include <libPPUI/wtl-pp.h>

#include "stdafx.h"
#include "ConfigGuids.h"
#include "ConfigBindings.h"

#include "ConfigWindow.fwd.h"

namespace coverflow {

class ConfigDialog : public CDialogImpl<ConfigDialog>, public preferences_page_instance,
  fb2k::CDarkModeHooks {

 public:
  enum { IDD = IDD_CONFIG_TABS };

  // Constructor - invoked by preferences_page_impl helpers - don't do Create() in here,
  // preferences_page_impl does this for us

  // Note that we don't bother doing anything regarding destruction* of our class.
  // The host ensures that our dialog is destroyed first, then the last reference
  // to our preferences_page_instance object is released, causing our object to be
  // deleted.

  // preferences_page_instance methods (not all of them - get_wnd() is
  // supplied by preferences_page_impl helpers)

  //* destructor is called to remember session active tab

  ConfigDialog(){}
  ~ConfigDialog(){}

  ConfigDialog(preferences_page_callback::ptr callback)
    : m_callback(callback) {}

  BEGIN_MSG_MAP_EX(ConfigDialog)
    MSG_WM_INITDIALOG(OnInitDialog)
    MSG_WM_NOTIFY(OnNotify)
    MSG_WM_DESTROY(OnDestroy)
  END_MSG_MAP()

  t_uint32 get_state() final;
  void apply() final;
  void reset() final;

  bool HasChanged();

  bool NeedReload() { return bneedreload; }
  bool NeedRedraw() { return bneedredraw; }
  void OnChanged();

 private:
  const preferences_page_callback::ptr m_callback;
  BindingCollection bindings_;
  bool bdiscarding = false;
  bool bneedreload = false;
  bool bneedredraw = false;
  ConfigWindow* m_configwindow = nullptr;
  t_size currentTab;

  bool m_loadingTabBindings = false;
  void LoadingBindings(bool loading) { m_loadingTabBindings = loading; }
  bool IsLoadingBindings() { return m_loadingTabBindings; }

  BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
  void OnDestroy();

 protected:
  HWND get_wnd() {
    return m_hWnd;
  }

  void BindControls(UINT_PTR ndx, HWND hWndTab, int cmd);
  void ClearBindings();
};

class preferences_page_myimpl : public preferences_page_impl<ConfigDialog> {

 public:
  const char* get_name() { return COMPONENT_NAME_LABEL;
  }

  GUID get_guid() {
    return guid_config_dialog;
  }

  GUID get_parent_guid() {
    //'display' fb2k preference section
    return guid_display;
  }
};

static preferences_page_factory_t<preferences_page_myimpl>
g_preferences_page_myimpl_factory;
}   //namespace coverflow
