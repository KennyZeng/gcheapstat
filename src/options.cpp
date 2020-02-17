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
    if (auto res = wcschr(argv[i], ':')) {
      auto next = res + 1;
      if (*next) val = next;
      *res = 0;
    }
    if (!_wcsicmp(argv[i], L"/pipe")) {
      if (!val)
        // pipe name is missing
        return -1;
      pipename = val;
    } else if (!_wcsicmp(argv[i], L"/pid") || !_wcsicmp(argv[i], L"/p")) {
      if (!val || swscanf_s(val, L"%u", &pid) != 1) {
        fprintf(stderr, "Invalid or missing PID value. ");
        return -1;
      }
    } else if (!_wcsicmp(argv[i], L"/limit") || !_wcsicmp(argv[i], L"/l")) {
      if (!val || swscanf_s(val, L"%zu", &limit) != 1) {
        fprintf(stderr, "Invalid or missing value for LIMIT option. ");
        return -1;
      }
    } else if (!_wcsicmp(argv[i], L"/sort") || !_wcsicmp(argv[i], L"/s")) {
      if (!val)
        // default sorting options apply
        continue;
      if (val[0] == '-' || val[0] == '+') {
        order = (val[0] == '+') ? Order::Asc : Order::Desc;
        ++val;
        if (*val == 0)
          // just sort order
          continue;
      }
      if (auto res = wcschr(val, ':')) {
        auto next = res + 1;
        if (*next && swscanf_s(next, L"%u", &orderby_gen) != 1) {
          fprintf(stderr, "Invalid generation number for SORT option. ");
          return -1;
        }
        *res = 0;
      }
      if (!_wcsicmp(val, L"size") || !_wcsicmp(val, L"s"))
        orderby = OrderBy::TotalSize;
      else if (!_wcsicmp(val, L"count") || !_wcsicmp(val, L"c"))
        orderby = OrderBy::Count;
      else {
        fprintf(stderr, "Invalid column name for SORT option. ");
        return -1;
      }
    } else if (!_wcsicmp(argv[i], L"/gen") || !_wcsicmp(argv[i], L"/g")) {
      if (!val || swscanf_s(val, L"%u", &gen) != 1) {
        fprintf(stderr, "Invalid or missing value for GEN option. ");
        return -1;
      }
    } else if (!_wcsicmp(argv[i], L"/help") || !_wcsicmp(argv[i], L"/h") ||
               !_wcsicmp(argv[i], L"/?"))
      help = true;
    else if (!_wcsicmp(argv[i], L"/verbose") || !wcscmp(argv[i], L"/V")) {
      if (!val || !_wcsicmp(val, L"yes") || !_wcsicmp(val, L"y"))
        verbose = true;
      else if (_wcsicmp(val, L"no") || !_wcsicmp(val, L"n")) {
        fprintf(stderr, "Invalid value for VERBOSE option. ");
        return -1;
      }
    } else if (!_wcsicmp(argv[i], L"/runas") || !_wcsicmp(argv[i], L"/as")) {
      if (!val) {
        fprintf(stderr, "Missing value for RUNAS option. ");
        return -1;
      }
      if (!_wcsicmp(val, L"localsystem"))
        runaslocalsystem = true;
      else {
        fprintf(stderr, "Invalid value for RUNAS option. ");
        return -1;
      }
    } else if (!_wcsicmp(argv[i], L"/version") || !wcscmp(argv[i], L"/v")) {
      version = true;
    } else {
      fwprintf(stderr, L"'%s' is not an option. ", argv[i]);
      return -1;
    }
  }
  return count;
}

void PrintVersion() { printf(PRODUCTNAME " version " VERSION "\n"); }

void PrintUsage(FILE* file) {
  fprintf(file, DESCRIPTION ".\n\n");
  char name[] = QTARGETNAME;
  // clang-format off
  std::transform(name, name + strlen(name), name, toupper);
  fprintf(file, "%s [/VERSION] [/HELP] [/VERBOSE] [/SORT:{+|-}{SIZE|COUNT}[:gen]]\n", name);
  std::fill_n(name, strlen(name), ' ');
  fprintf(file, "%s [/LIMIT:count] [/GEN:gen] [/RUNAS:LOCALSYSTEM] /PID:pid\n\n", name);
  fprintf(file, "  HELP     Display usage information.\n");
  fprintf(file, "  VERSION  Display version.\n");
  fprintf(file, "  VERBOSE  Display warning and information messages while running.\n");
  fprintf(file, "           Only errors are displayed by default.\n");
  fprintf(file, "  SORT     Sort output by either total SIZE or COUNT, ascending '+' or\n");
  fprintf(file, "           descending '-'. You can also specify generation to sort on.\n");
  fprintf(file, "  LIMIT    Limit the number of rows to output.\n");
  fprintf(file, "  GEN      Count only objects of the generation specified.\n");
  fprintf(file, "  RUNAS    The only currently available value is LOCALSYSTEM (run under\n");
  fprintf(file, "           LocalSystem computer account). This is to allow inspection of managed\n");
  fprintf(file, "           services running under LocalSystem (administrator account is not\n");
  fprintf(file, "           powerful enough for that).\n");
  fprintf(file, "  PID      Target process ID.\n");
  // clang-format on
}
