#include "options.h"

#include "version.h"

int Options::ParseCommandLine(PCWSTR cmdline) {
  int argc = 0;
  auto argv = CommandLineToArgvW(cmdline, &argc);
  if (argv == nullptr) return -1;
  std::unique_ptr<PWSTR, decltype(&LocalFree)> argv_scope_guard{
      CommandLineToArgvW(cmdline, &argc), LocalFree};
  int count = 0;
  for (auto i = 1; i < argc; ++i, ++count) {
    PWSTR val = nullptr;
    if (auto res = wcschr(argv[i], '=')) {
      auto next = res + 1;
      if (*next) val = next;
      *res = 0;
    }
    if (!wcscmp(argv[i], L"--pipe")) {
      if (!val) return -1;  // pipe name is missing
      pipename = val;
    } else if (!wcscmp(argv[i], L"--pid")) {
      if (!val || swscanf_s(val, L"%u", &pid) != 1)
        return false;  // pid is missing or invalid
    } else if (!wcscmp(argv[i], L"--limit")) {
      if (!val || swscanf_s(val, L"%zu", &limit) != 1)
        return false;  // limit is missing or invalid
    } else if (!wcscmp(argv[i], L"--sort")) {
      if (!val) continue;  // default sorting options apply
      if (val[0] == '-') {
        order = Order::Desc;
        ++val;
        if (*val == 0) continue;  // reverse sort order only
      }
      if (!wcsncmp(val, L"size", 4)) {
        orderby = OrderBy::TotalSize;
        val += 4;
      } else if (!wcsncmp(val, L"count", 5)) {
        orderby = OrderBy::Count;
        val += 5;
      } else {
        return -1;  // invalid field name to sort on
      }
      if (val[0] == ':') {
        if (swscanf_s(val + 1, L"%u", &orderby_gen) != 1)
          return -1;  // invalid generation number to sort on
      } else if (val[0])
        return -1;  // unrecognized string after field name
    } else if (!wcscmp(argv[i], L"--gen")) {
      if (swscanf_s(val, L"%u", &gen) != 1)
        return -1;  // invalid generation number to display
    } else if (!wcscmp(argv[i], L"--help")) {
      help = true;
    } else if (!wcscmp(argv[i], L"--verbose")) {
      verbose = true;
    } else if (!wcscmp(argv[i], L"--runas")) {
      if (!val || wcsncmp(val, L"localsystem", 11))
        return -1;  // account name is missing or unsupported
      runaslocalsystem = true;
    } else if (!wcscmp(argv[i], L"--version")) {
      version = true;
    } else {
      return -1;  // invalid option
    }
  }
  return count;
}

void PrintVersion() { printf("gcheapstat version " VERSION "\n"); }

void PrintUsage(FILE* file) {
  fprintf(file,
          "usage: gcheapstat [--verbose] [--sort=(size|count)[:<gen>]] "
          "[--limit=<count>] [--gen=<gen>] --pid=<pid>\n");
}