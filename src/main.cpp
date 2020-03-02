#include "application.h"
#include "options.h"
#include "rpc_server.h"
#include "version.h"

wchar_t Buffer[64 * 1024];

int main() {
  Options options{};
  auto count = options.ParseCommandLine(GetCommandLineW());
  if (count < 0) {
    // Error parsing command-line arguments
    fprintf(stderr, "See '" QTARGETNAME " /help'.\n");
    return 1;
  }
  if (options.help) {
    // Asking for a help
    PrintUsage();
    return 0;
  }
  Log::Verbose = options.verbose;
  if (options.pipename) {
    // Running RPC server mode
    Log::Mode = LogMode::Pipe;
    auto hr = RpcServer{Buffer}.Run(options.pipename);
    return FAILED(hr) ? 1 : 0;
  }
  if (options.version) {
    PrintVersion();
    // Asking for just a version is OK
    if (count == 1) return 0;
  }
  if (!options.pid) {
    fprintf(stderr,
            "Process ID of the target application is not specified. See "
            "'" QTARGETNAME " /help'.\n");
    return 1;
  }
  // Go
  Log::Mode = LogMode::Console;
  auto hr = Application{Buffer}.Run(options);
  if (FAILED(hr)) {
    auto len = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, hr,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), Buffer,
        _ARRAYSIZE(Buffer), NULL);
    if (len) {
      wprintf(Buffer);
    } else
      wprintf(L"Error 0x%08lx\n", hr);
    return 1;
  }
  return 0;
}

PVOID __RPC_API MIDL_user_allocate(size_t size) { return malloc(size); }
void __RPC_API MIDL_user_free(PVOID p) { free(p); }
