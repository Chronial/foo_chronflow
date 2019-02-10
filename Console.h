#pragma once

class Console {
  Console() = delete;

 public:
  static void create();
  static void print(const wchar_t* str);
  static void println(const wchar_t* str);
  static void printf(const wchar_t* format, ...);

 private:
  static HANDLE screenBuffer;
};
