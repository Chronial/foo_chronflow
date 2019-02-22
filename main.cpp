#include "ContainerWindow.h"
#include "utils.h"

#define VERSION "0.4.5"

// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
DECLARE_COMPONENT_VERSION("Chronial's Coverflow", VERSION,
                          "Renders Album Art in a 3d environment\n"
                          "By Christian Fersch\n"
                          "Cover image by figure002 (http://fav.me/d2ppgwx)\n" __DATE__
                          " - " __TIME__);

VALIDATE_COMPONENT_FILENAME("foo_chronflow.dll");

class QuitHandler : public initquit {
 public:
  void on_quit() final { glfwTerminate(); }
};
static service_factory_single_t<QuitHandler> quitHandler;
