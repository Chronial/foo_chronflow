#pragma once
#include "engine.fwd.h"
#include "utils.h"

namespace engine {

class FindAsYouType {
  inline static const double typeTimeout = 1.0;
  pfc::string8 enteredString;
  std::optional<Timer> timeoutTimer;

  class Engine& engine;

 public:
  explicit FindAsYouType(Engine& engine) : engine(engine){};
  void onChar(WPARAM wParam);
  void reset();
  std::vector<size_t> highlightPositions(const std::string& albumTitle);

 private:
  void enterChar(wchar_t c);
  void removeChar();
  bool updateSearch(const char* searchFor);
};

class FuzzyMatcher {
 public:
  explicit FuzzyMatcher(const std::string& pattern);
  int match(const std::string& input, std::vector<size_t>* positions = nullptr);

 private:
  std::wstring pattern;
};

} // namespace
