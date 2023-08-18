#pragma warning(push, 1)
#include "../columns_ui-sdk/ui_extension.h"
#pragma warning(pop)

#include "ContainerWindow.h"
#include "style_manager.h"

namespace {

using engine::ContainerWindow;
using render::StyleManager;

class StyleCallback : public columns_ui::colours::common_callback,
                      public columns_ui::fonts::common_callback {
  static_api_ptr_t<cui::colours::manager> colorManager;
  static_api_ptr_t<cui::fonts::manager> fontManager;
  std::function<void()> callback = nullptr;

 public:
  StyleCallback(std::function<void()> callback) : callback(callback) {
    colorManager->register_common_callback(this);
    fontManager->register_common_callback(this);
  }

  ~StyleCallback() {
    colorManager->deregister_common_callback(this);
    fontManager->deregister_common_callback(this);
  }

  virtual void on_colour_changed(uint32_t changed_items_mask) const { callback(); }
  virtual void on_bool_changed(uint32_t changed_items_mask) const final { callback(); }
  virtual void on_font_changed(uint32_t changed_items_mask) const final { callback(); }

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

class cui_coverflow : public ui_extension::window {
  ui_extension::window_host_ptr m_host;
  std::optional<ContainerWindow> window;
  std::optional<CuiStyleManager> style_manager;

 public:
  cui_coverflow() = default;

  void get_category(pfc::string_base& out) const final { out = "Panels"; }

  const GUID& get_extension_guid() const final {
    // {3C880108-E9A8-454F-AB82-22B49D6BA105}
    static const GUID guid_foo_coverflow = {
        0x3c880108, 0xe9a8, 0x454f, {0xab, 0x82, 0x22, 0xb4, 0x9d, 0x6b, 0xa1, 0x5}}; //modded
    return guid_foo_coverflow;
  }

  void get_name(pfc::string_base& out) const final { out = COMPONENT_NAME_LABEL; }

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
            << AppNameInternal << " panel failed to initialize: " << e.what();
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

  void set_config(stream_reader* p_reader, t_size p_size,
                  abort_callback& p_abort) override {

    stream_reader_formatter<false> reader(*p_reader, p_abort);
    window->set_uicfg(&reader, p_size, p_abort);

  };

  void get_config(stream_writer* p_writer, abort_callback& p_abort) const override {

    stream_writer_formatter<false> writer(*p_writer, p_abort);
    window->get_uicfg(&writer, p_abort);

  };

  HWND get_wnd() const final { return window->getHWND(); }
  const bool get_is_single_instance() const final { return false; }
};

static service_factory_t<cui_coverflow> cui_coverflow_instance;

} // namespace
