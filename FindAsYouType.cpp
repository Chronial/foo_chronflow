#include "FindAsYouType.h"
#include "Engine.h"
#include "EngineThread.h"
#include "PlaybackTracer.h"

namespace engine {
using coverflow::configData;
// The fuzzy matching algorithm is adapted from fzf
namespace {

using index_t = t_uint8;
using score_t = t_int16;

namespace {

const score_t scoreMatch = 16;
const score_t penaltyGapStart = -8;
const score_t penaltyGapExtention = -1;

// We prefer matches at the beginning of a word, but the bonus should not be
// too great to prevent the longer acronym matches from always winning over
// shorter fuzzy matches.
const score_t bonusBoundary = 8;

// Although bonus point for non-word characters is non-contextual, we need it
// for computing bonus points for consecutive chunks starting with a non-word
// character.
const score_t bonusNonWord = 0;

// Minimum bonus point given to characters in consecutive chunks.
// Note that bonus points for consecutive matches shouldn't have needed if
// we used fixed match score as in the original algorithm.
const score_t bonusConsecutive = -(penaltyGapStart + penaltyGapExtention);

// The first character in the typed pattern usually has more
// significance than the rest so it's important that it appears at
// special positions where bonus points are given. e.g. "to-go" vs.
// "ongoing" on "og" or on "ogo". The amount of the extra bonus should
// be limited so that the gap penalty is still respected.
const score_t bonusFirstCharMultiplier = 2;

}  // namespace

enum charClass { charNonWord, charAlnum };

charClass getCharClass(wchar_t c) {
  if (IsCharAlphaNumericW(c)) {
    return charAlnum;
  } else {
    return charNonWord;
  }
}

score_t bonusFor(charClass prevClass, charClass cClass) {
  if (prevClass == charNonWord && cClass != charNonWord) {
    return bonusBoundary;
  } else if (cClass == charNonWord) {
    return bonusNonWord;
  }
  return 0;
}

int fuzzy_match(const std::wstring& pattern, const std::string& input,
                std::vector<size_t>* positions) {
  // Assume that pattern is given in lowercase
  // First check if there's a match and calculate bonus for each position.
  index_t M = index_t(pattern.length());
  if (M == 0) {
    return 0;
  }

  // Rune array
  std::vector<wchar_t> T(
      std::min(input.length() + 1, size_t(std::numeric_limits<index_t>::max())));
  index_t N = index_t(pfc::stringcvt::convert_utf8_to_wide(
      T.data(), T.size(), input.c_str(), input.size()));
  T.resize(N);
  CharLowerW(T.data());
  for (auto& c : T) {
    if (c == '\r' || c == '\n')
      c = ' ';
  }

  // Phase 1. Optimized search for ASCII string
  // First row of score matrix
  std::vector<score_t> H0(N);
  std::vector<index_t> C0(N);

  // The first occurrence of each character in the pattern
  std::vector<index_t> F(M);

  // Bonus point for each position
  std::vector<score_t> B(N);

  // Phase 2. Calculate bonus for each point
  score_t maxScore = 0;
  index_t maxScorePos = 0;
  index_t pidx = 0;

  // Will hold the last index of pattern[-1] in input
  index_t lastIdx = 0;
  wchar_t pchar0 = pattern[0];
  wchar_t pchar = pattern[0];
  score_t prevH0 = 0;

  auto prevClass = charNonWord;
  bool inGap = false;
  for (index_t off = 0; off < T.size(); off++) {
    wchar_t c = T[off];
    charClass cClass = getCharClass(pchar);
    score_t bonus = bonusFor(prevClass, cClass);
    B[off] = bonus;
    prevClass = cClass;

    if (c == pchar) {
      if (pidx < M) {
        F[pidx] = off;
        pidx++;
        pchar = pattern[std::min(pidx, index_t(M - 1))];
      }
      lastIdx = off;
    }

    if (c == pchar0) {
      score_t score = scoreMatch + bonus * bonusFirstCharMultiplier;
      H0[off] = score;
      C0[off] = 1;
      if (M == 1 && score > maxScore) {
        maxScore = score;
        maxScorePos = off;
        if (bonus == bonusBoundary) {
          break;
        }
      }
      inGap = false;
    } else {
      if (inGap) {
        H0[off] = score_t(std::max(0, prevH0 + penaltyGapExtention));
      } else {
        H0[off] = score_t(std::max(0, prevH0 + penaltyGapStart));
      }
      C0[off] = 0;
      inGap = true;
    }
    prevH0 = H0[off];
  }
  if (pidx != M) {
    return -1;
  }
  if (M == 1) {
    if (positions) {
      *positions = std::vector<size_t>{maxScorePos};
    }
    return maxScore;
  }

  // Phase 3. Fill in score matrix
  index_t f0 = F[0];
  int width = lastIdx - f0 + 1;
  // score matrix
  std::vector<score_t> H(width * M);
  std::copy(&H0[f0], &H0[lastIdx] + 1, &H[0]);

  // Possible length of consecutive chunk at each position.
  std::vector<index_t> C(width * M);
  std::copy(&C0[f0], &C0[lastIdx] + 1, &C[0]);

  for (index_t i = 1; i < M; i++) {
    int row = i * width;
    index_t f = F[i];
    inGap = false;
    for (index_t j = f; j <= lastIdx; j++) {
      index_t j0 = j - f0;
      // score if we "go diagonal"
      score_t s1 = 0;
      // s2 is score if we don't consume a pattern character
      score_t s2 = 0;
      index_t consecutive = 0;

      if (j > f) {
        if (inGap) {
          s2 = H[row + j0 - 1] + penaltyGapExtention;
        } else {
          s2 = H[row + j0 - 1] + penaltyGapStart;
        }
      }

      if (pattern[i] == T[j]) {
        score_t b = B[j];
        consecutive = C[row - width + j0 - 1] + 1;
        // Break consecutive chunk
        if (b == bonusBoundary) {
          consecutive = 1;
        } else if (consecutive > 1) {
          b = std::max({b, bonusConsecutive, B[j - int(consecutive) + 1]});
        }
        s1 = H[row - width + j0 - 1] + scoreMatch + b;
        if (s1 < s2) {
          consecutive = 0;
        }
      }
      C[row + j0] = consecutive;

      score_t score = std::max({score_t(0), s1, s2});
      H[row + j0] = score;
      if (i == M - 1 && score > maxScore) {
        maxScore = score;
        maxScorePos = j;
      }
      inGap = s1 < s2;
    }
  }

  // Phase 4. (Optional) Backtrace to find character positions
  if (positions) {
    positions->clear();
    index_t j = maxScorePos;
    int i = M - 1;
    bool preferMatch = true;
    while (true) {
      int row = i * width;
      index_t j0 = j - f0;
      score_t s = H[row + j0];
      score_t s1 = 0;
      score_t s2 = 0;
      if (i > 0 && j >= F[i]) {
        s1 = H[row - width + j0 - 1];
      }
      if (j > F[i]) {
        s2 = H[row + j0 - 1];
      }

      if (s > s1 && (s > s2 || s == s2 && preferMatch)) {
        positions->push_back(j);
        if (i == 0) {
          break;
        }
        i--;
      }
      preferMatch = C[row + j0] > 1 ||
                    row + width + j0 + 1 < int(C.size()) && C[row + width + j0 + 1] > 0;
      j--;
    }
    std::reverse(positions->begin(), positions->end());
  }
  return maxScore;
}

}  // namespace

FuzzyMatcher::FuzzyMatcher(const std::string& pattern) {
  this->pattern = wstring_from_utf8(pattern);
  this->pattern.resize(
      std::min(int(this->pattern.size()), std::numeric_limits<index_t>::max() - 1));
  CharLowerW(this->pattern.data());
}

int FuzzyMatcher::match(const std::string& input, std::vector<size_t>* positions) {
  return fuzzy_match(this->pattern, input, positions);
}

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

namespace {
void fold_whitespace(std::string& s) {
  bool prev_whitespace = true;
  int j = 0;
  for (const char& c : s) {
    if (c == ' ' || c == '\r' || c == '\n') {
      if (prev_whitespace) {
        continue;
      } else {
        prev_whitespace = true;
        s[j++] = ' ';
      }
    } else {
      prev_whitespace = false;
      s[j++] = c;
    }
  }
  s.resize(j);
}
}  // namespace

void FindAsYouType::enterChar(wchar_t c) {
  std::string newString(enteredString.c_str());
  newString.append(pfc::stringcvt::string_utf8_from_wide(&c, 1));
  fold_whitespace(newString);
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

std::vector<size_t> FindAsYouType::highlightPositions(const std::string& albumTitle) {
  std::vector<size_t> positions;
  if (enteredString.is_empty()) {
    return positions;
  }
  FuzzyMatcher matcher{std::string(enteredString)};
  matcher.match(albumTitle, &positions);
  return positions;
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
} // namespace
