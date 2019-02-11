#pragma once
#include "utils.h"

class FindAsYouType {
  inline static const double typeTimeout = 1.0;
  pfc::string8 enteredString;
  std::optional<Timer> timeoutTimer;

  class Engine& engine;

 public:
  explicit FindAsYouType(Engine& engine) : engine(engine){};
  void onChar(WPARAM wParam);

 private:
  void enterChar(wchar_t c);
  void removeChar();
  void reset();
  bool updateSearch(const char* searchFor);
};
