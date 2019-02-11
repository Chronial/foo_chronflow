#include "ContainerWindow.h"
#include "Helpers.h"

#define VERSION "0.4.5"

// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
DECLARE_COMPONENT_VERSION("Chronial's Coverflow", VERSION,
                          "Renders Album Art in a 3d environment\n"
                          "By Christian Fersch\n"
                          "Cover image by figure002 (http://fav.me/d2ppgwx)\n" __DATE__
                          " - " __TIME__);

VALIDATE_COMPONENT_FILENAME("foo_chronflow.dll");

class InitHandler : public init_stage_callback {
 public:
  void on_init_stage(t_uint32 stage) final {
    if (stage == init_stages::after_library_init) {
      initGlfw();
    } else if (stage == init_stages::before_ui_init) {
      registerWindowClasses();
    }
  }

 private:
  void initGlfw() {
    glfwSetErrorCallback([](int error, const char* description) {
      pfc::string8 msg = "foo_chronflow glfw error: ";
      msg.add_string(description);
      console::print(msg);
    });
    if (!glfwInit()) {
      console::print("foo_chronflow failed to initialize glfw.");
      // TODO: Handle this somehow
    }
  }
  void registerWindowClasses() {
    // Note: We do not need to unregister these classes as it happens automatically when
    // foobar quits
    if (!ContainerWindow::registerWindowClass()) {
      errorPopupWin32("Failed to register MainWindow class");
    }
  }
};
static service_factory_single_t<InitHandler> initHandler;

class QuitHandler : public initquit {
 public:
  void on_quit() final { glfwTerminate(); }
};
static service_factory_single_t<QuitHandler> quitHandler;
