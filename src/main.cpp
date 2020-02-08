#include <fcntl.h>
#include <io.h>

#include "application.h"
#include "options.h"
#include "rpc_server.h"

LoggerMode Mode{LoggerMode::None};
wchar_t Buffer[64 * 1024];

int main() {
  Options options{};
  if (!options.ParseCommandLine(GetCommandLineW())) {
    PrintUsage(stderr);
    return 1;
  }
  _setmode(_fileno(stdout), _O_U16TEXT);
  if (options.pipename) {
    Mode = LoggerMode::RpcServer;
    auto hr = RpcServer{Buffer}.Run(options.pipename);
    return FAILED(hr) ? 1 : 0;
  }
  if (!options.pid) {
    if (options.help) {
      PrintUsage(stdout);
      return 0;
    } else {
      PrintUsage(stderr);
      return 1;
    }
  }
  Mode = LoggerMode::Console;
  auto hr = Application{}.Run(options);
  if (FAILED(hr)) {
    auto len = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, hr,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), Buffer,
        _ARRAYSIZE(Buffer), NULL);
    if (len) {
      wprintf(Buffer);
      wprintf(L"\n");
    } else
      wprintf(L"Error 0x%08lx\n", hr);
    return 1;
  }
  return 0;
}

PVOID __RPC_API MIDL_user_allocate(size_t size) { return malloc(size); }
void __RPC_API MIDL_user_free(PVOID p) { free(p); }