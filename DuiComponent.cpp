
#include <utility>

#include "ContainerWindow.h"
#include "style_manager.h"

namespace {

class DuiStyleManager : public StyleManager {
 public:
  DuiStyleManager(ui_element_instance_callback::ptr instance_callback)
      : instance_callback(instance_callback) {
    updateCache();
  }
  virtual COLORREF defaultTitleColor() final {
    return instance_callback->query_std_color(ui_color_text);
  }
  virtual LOGFONT defaultTitleFont() final {
    LOGFONT font = {};
    HFONT hfont = instance_callback->query_font_ex(ui_font_default);
    check(GetObject(hfont, sizeof(font), &font));
    return font;
  }
  virtual COLORREF defaultBgColor() final {
    return instance_callback->query_std_color(ui_color_background);
  }

 private:
  ui_element_instance_callback::ptr instance_callback;
};

static const GUID guid_dui_foo_chronflow = {
    0x1d56881c, 0xca24, 0x470c, {0x94, 0x4a, 0xde, 0xd8, 0x30, 0xf9, 0xe9, 0x5d}};
static const GUID element_subclass = ui_element_subclass_media_library_viewers;

class dui_chronflow : public ui_element_instance {
  ui_element_config::ptr config;
  DuiStyleManager style_manager;
  ContainerWindow window;

 public:
  dui_chronflow(HWND parent, ui_element_config::ptr config,
                ui_element_instance_callback_ptr p_callback)
      : config(std::move(config)), style_manager(p_callback),
        window(parent, style_manager, p_callback) {}
  HWND get_wnd() final { return window.getHWND(); };

  static void g_get_name(pfc::string_base& out) { out = "Coverflow"; }
  static const char* g_get_description() {
    return "Displays a 3D rendering of the Album Art in your Media Library";
  }

  virtual void notify(const GUID& p_what, t_size p_param1, const void* p_param2,
                      t_size p_param2size) {
    if (p_what == ui_element_notify_colors_changed ||
        p_what == ui_element_notify_font_changed) {
      style_manager.onChange();
    }
  }

  GUID get_guid() final { return guid_dui_foo_chronflow; }
  GUID get_subclass() final { return element_subclass; }

  void set_configuration(ui_element_config::ptr config) final { this->config = config; }
  ui_element_config::ptr get_configuration() final { return config; }
};

class UiElement : public ui_element {
 public:
  GUID get_guid() final { return guid_dui_foo_chronflow; }
  GUID get_subclass() final { return element_subclass; }
  void get_name(pfc::string_base& out) final { dui_chronflow::g_get_name(out); }
  ui_element_instance::ptr instantiate(HWND parent, ui_element_config::ptr cfg,
                                       ui_element_instance_callback::ptr callback) final {
    PFC_ASSERT(cfg->get_guid() == get_guid());
    service_nnptr_t<dui_chronflow> item =
        new service_impl_t<dui_chronflow>(parent, cfg, callback);
    return item;
  }
  ui_element_config::ptr get_default_configuration() final {
    return ui_element_config::g_create_empty(guid_dui_foo_chronflow);
  }
  ui_element_children_enumerator_ptr enumerate_children(
      ui_element_config::ptr /*cfg*/) final {
    return nullptr;
  }
  bool get_description(pfc::string_base& out) final {
    out = dui_chronflow::g_get_description();
    return true;
  }
};

static service_factory_single_t<UiElement> uiElement;

}  // namespace
