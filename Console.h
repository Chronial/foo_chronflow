#pragma once

struct Console {
  Console() = delete;
  static void create();
  static void print(const wchar_t* str);
  static void println(const wchar_t* str);
  static void printf(const wchar_t* format, ...);

 private:
  static HANDLE screenBuffer;
};
