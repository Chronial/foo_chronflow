#include "utils.h"

#include <cstdio>

void errorPopup(const char* message) {
  // This should be:
  // popup_message::g_show(... , popup_message::icon_error);
  // But we sometimes need this to be modal (as it will be followed by crash)
  MessageBoxW(nullptr,
              uT(PFC_string_formatter()
                 << "foo_chronflow: " << message
                 << "\r\n\r\nIf this happens more than once, please report this error "
                    "in the foo_chronflow "
                    "thread on Hydrogenaudo or via mail to foocomp@chronial.de"),
              L"Error in foo_chronflow", MB_OK | MB_ICONERROR);
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

// adjusts a given path for certain discrepancies between how foobar2000
// and GDI+ handle paths, and other oddities
//
// Currently fixes:
//   - User might use a forward-slash instead of a
//     backslash for the directory separator
//   - GDI+ ignores trailing periods '.' in directory names
//   - GDI+ and FindFirstFile ignore double-backslashes
//   - makes relative paths absolute to core_api::get_profile_path()
// Copied from  foo_uie_albumart
void Helpers::fixPath(pfc::string_base& path) {
  if (path.get_length() == 0)
    return;

  pfc::string8 temp;
  titleformat_compiler::remove_forbidden_chars_string(temp, path, ~0u, "*?<>|\"");

  // fix directory separators
  temp.replace_char('/', '\\');

  bool is_unc = (pfc::strcmp_partial(temp, "\\\\") == 0);
  if ((temp[1] != ':') && (!is_unc)) {
    pfc::string8 profilePath;
    filesystem::g_get_display_path(core_api::get_profile_path(), profilePath);
    profilePath.add_byte('\\');

    temp.insert_chars(0, profilePath);
  }

  // fix double-backslashes and trailing periods in directory names
  t_size temp_len = temp.get_length();
  path.reset();
  path.add_byte(temp[0]);
  for (t_size n = 1; n < temp_len - 1; n++) {
    if (temp[n] == '\\') {
      if (temp[n + 1] == '\\')
        continue;
    } else if (temp[n] == '.') {
      if ((temp[n - 1] != '.' && temp[n - 1] != '\\') && temp[n + 1] == '\\')
        continue;
    }
    path.add_byte(temp[n]);
  }
  if (temp_len > 1)
    path.add_byte(temp[temp_len - 1]);
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
