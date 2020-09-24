#pragma once
#include "utils.h"

class graphics_reset : public std::exception {
  char const* what() const noexcept override { return "graphics driver reset"; }
};

class GLContext {
 public:
  explicit GLContext(class EngineWindow&);
  NO_MOVE_NO_COPY(GLContext);
  ~GLContext();

  static void checkGraphicsReset();

 private:
  bool wasReset = false;
  static thread_local GLContext* currentContext;
};
