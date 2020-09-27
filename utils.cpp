#include "utils.h"

#include <cstdio>

void errorPopup(const char* message) {
  // This should be:
  // popup_message::g_show(... , popup_message::icon_error);
  // But we sometimes need this to be modal (as it will be followed by crash)
  MessageBoxW(nullptr,
              uT(PFC_string_formatter()
                 << AppNameInternal << ": " << message
                 << "\r\n\r\nIf this happens more than once, please report this error "
                    "in the " << AppNameInternal <<
                    " thread on Hydrogenaudo or via mail to " << AppEMail),
              uT(PFC_string_formatter() << "Error in " << AppNameInternal), MB_OK | MB_ICONERROR);
}

void errorPopupWin32(const char* message) {
  errorPopup(PFC_string_formatter() << message << "\r\nWin32 Error Message: "
                                    << format_win32_error(GetLastError()));
}

std::string linux_lineendings(std::string s) {
  boost::replace_all(s, "\r\n", "\n");
  return std::move(s);
}

std::string windows_lineendings(std::string s) {
  boost::replace_all(s, "\r\n", "\n");
  boost::replace_all(s, "\n", "\r\n");
  return std::move(s);
}

double time() {
  static std::array<t_int64, 2> resolution_and_offset = [] {
    LARGE_INTEGER resolution;
    QueryPerformanceFrequency(&resolution);
    LARGE_INTEGER offset;
    QueryPerformanceCounter(&offset);
    return std::array<t_int64, 2>{resolution.QuadPart, offset.QuadPart};
  }();
  LARGE_INTEGER count;
  QueryPerformanceCounter(&count);
  return double(count.QuadPart - resolution_and_offset[1]) / resolution_and_offset[0];
}

#ifdef _DEBUG
namespace console {
out::out() {
  *this << " " << std::fixed << std::setprecision(2);
}
out::~out() {
  static HANDLE console = [] {
    AllocConsole();
    return GetStdHandle(STD_OUTPUT_HANDLE);
  }();
  *this << "\n";
  auto str = pfc::stringcvt::string_wide_from_utf8(this->str().c_str());
  WriteConsole(console, str.get_ptr(), str.length(), nullptr, nullptr);
}
}  // namespace console
#endif
