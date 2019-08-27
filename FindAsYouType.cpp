#include "FindAsYouType.h"

#include "DbAlbumCollection.h"
#include "Engine.h"
#include "EngineThread.h"
#include "PlaybackTracer.h"

void FindAsYouType::onChar(WPARAM wParam) {
  switch (wParam) {
    case 1:  // any other nonchar character
    case 0x09:  // Process a tab.
      break;

    case 0x08:  // Process a backspace.
      removeChar();
      break;

    case 0x0A:  // Process a linefeed.
    case 0x0D:  // Process a carriage return.
    case 0x1B:  // Process an escape.
      reset();
      break;

    default:  // Process any writeable character
      enterChar(static_cast<wchar_t>(wParam));
      break;
  }
}

void FindAsYouType::enterChar(wchar_t c) {
  std::string newString(enteredString.c_str());
  newString.append(pfc::stringcvt::string_utf8_from_wide(&c, 1));
  newString = remove_whitespace(newString);
  if (updateSearch(newString.c_str())) {
    enteredString = newString.c_str();
  } else {
    MessageBeep(0xFFFFFFFF);
  }
}

void FindAsYouType::removeChar() {
  enteredString.truncate(
      pfc::utf8_chars_to_bytes(enteredString, pfc::strlen_utf8(enteredString) - 1));
  if (enteredString.length() > 0) {
    updateSearch(enteredString);
  }
}

void FindAsYouType::reset() {
  timeoutTimer.reset();
  enteredString.reset();
  engine.thread.invalidateWindow();
}

int FindAsYouType::highlightLength(const std::string& albumTitle) {
  size_t input_length = pfc::strlen_utf8(enteredString);
  std::string substring{};
  substring.reserve(albumTitle.length());
  for (auto c : albumTitle) {
    substring.push_back(c);
    if (pfc::strlen_utf8(remove_whitespace(substring).c_str()) == input_length) {
      return pfc::strlen_utf8(substring.c_str());
    }
  }
  return 0;
}

bool FindAsYouType::updateSearch(const char* searchFor) {
  timeoutTimer.reset();
  engine.playbackTracer.delay(typeTimeout);
  auto result = engine.db.performFayt(searchFor);
  if (result)
    engine.setTarget(result.value(), true);

  timeoutTimer.emplace(
      typeTimeout, [&] { engine.thread.send<EM::Run>([&] { reset(); }); });
  return result.has_value();
}
