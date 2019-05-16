#pragma warning(push, 1)
#include "../columns_ui-sdk/ui_extension.h"
#pragma warning(pop)

#include "ContainerWindow.h"
#include "style_manager.h"

namespace {

class StyleCallback : public columns_ui::colours::common_callback,
                      public columns_ui::fonts::common_callback {
  static_api_ptr_t<cui::colours::manager> colorManager;
  static_api_ptr_t<cui::fonts::manager> fontManager;
  std::function<void()> callback;

 public:
  StyleCallback(std::function<void()> callback) : callback(callback) {
    colorManager->register_common_callback(this);
    fontManager->register_common_callback(this);
  }

  ~StyleCallback() {
    colorManager->deregister_common_callback(this);
    fontManager->deregister_common_callback(this);
  }

  void on_colour_changed(t_size mask) const final { callback(); }
  void on_bool_changed(t_size mask) const final { callback(); }
  void on_font_changed(t_size mask) const final { callback(); }
};

class CuiStyleManager : public StyleManager {
  StyleCallback callback;

 public:
  CuiStyleManager() : callback([&] { this->onChange(); }) { updateCache(); }

  virtual COLORREF defaultTitleColor() final {
    return columns_ui::colours::helper(GUID{}).get_colour(
        columns_ui::colours::colour_text);
  }
  virtual LOGFONT defaultTitleFont() final {
    LOGFONT font;
    static_api_ptr_t<cui::fonts::manager> fontManager;
    fontManager->get_font(columns_ui::fonts::font_type_items, font);
    return font;
  }
  virtual COLORREF defaultBgColor() final {
    return columns_ui::colours::helper(GUID{}).get_colour(
        columns_ui::colours::colour_background);
  }
};

class cui_chronflow : public ui_extension::window {
  ui_extension::window_host_ptr m_host;
  std::optional<ContainerWindow> window;
  std::optional<CuiStyleManager> style_manager;

 public:
  cui_chronflow() = default;

  void get_category(pfc::string_base& out) const final { out = "Panels"; }

  const GUID& get_extension_guid() const final {
    // {DA317C70-654F-4A75-A4F2-10F4873773FC}
    static const GUID guid_foo_chronflow = {
        0xda317c70, 0x654f, 0x4a75, {0xa4, 0xf2, 0x10, 0xf4, 0x87, 0x37, 0x73, 0xfc}};
    return guid_foo_chronflow;
  }

  void get_name(pfc::string_base& out) const final { out = "Chronflow"; }

  unsigned get_type() const final { return ui_extension::type_panel; }

  bool is_available(const ui_extension::window_host_ptr& p_host) const final {
    return !m_host.is_valid() || p_host->get_host_guid() != m_host->get_host_guid();
  }

  HWND create_or_transfer_window(HWND wnd_parent,
                                 const ui_extension::window_host_ptr& p_host,
                                 const ui_helpers::window_position_t& p_position) final {
    if (!window || !window->getHWND()) {
      try {
        if (!style_manager)
          style_manager.emplace();
        window.emplace(wnd_parent, *style_manager);
      } catch (std::exception& e) {
        FB2K_console_formatter()
            << "foo_chronflow panel failed to initialize: " << e.what();
        return nullptr;
      }
      m_host = p_host;
      ShowWindow(window->getHWND(), SW_HIDE);
      SetWindowPos(window->getHWND(), nullptr, p_position.x, p_position.y, p_position.cx,
                   p_position.cy, SWP_NOZORDER);
    } else {
      ShowWindow(window->getHWND(), SW_HIDE);
      SetParent(window->getHWND(), wnd_parent);
      SetWindowPos(window->getHWND(), nullptr, p_position.x, p_position.y, p_position.cx,
                   p_position.cy, SWP_NOZORDER);
      m_host->relinquish_ownership(window->getHWND());
      m_host = p_host;
    }

    return window->getHWND();
  }

  void destroy_window() final {
    window.reset();
    m_host.release();
  }

  HWND get_wnd() const final { return window->getHWND(); }
  const bool get_is_single_instance() const final { return true; }
};

static service_factory_single_t<cui_chronflow> cui_chronflow_instance;

}  // namespace
