#include "Console.h"

#include <cstdio>

HANDLE Console::screenBuffer = nullptr;

void Console::create() {
  AllocConsole();
  screenBuffer = GetStdHandle(STD_OUTPUT_HANDLE);
}

void Console::print(const wchar_t* str) {
  WriteConsole(screenBuffer, str, wcslen(str), nullptr, nullptr);
}

void Console::println(const wchar_t* str) {
  print(str);
  print(L"\n");
}
void Console::printf(const wchar_t* format, ...) {
  va_list args;
  va_start(args, format);  // NOLINT
  wchar_t out[1024];
  int len = vswprintf_s(out, 1024, format, args);  // NOLINT
  WriteConsole(screenBuffer, out, len, nullptr, nullptr);  // NOLINT
}
