#pragma once
#include "rpc_h.h"  // MtStat

struct IMtNameProvider {
  // No virtual destructor intentionally
  virtual HRESULT GetMtName(uintptr_t addr, uint32_t size, PWSTR name,
                            uint32_t *needed) = 0;
};

inline bool IsUnicodeString(PCWSTR buffer, size_t size) {
  size_t len = 0;
  auto hr = StringCchLengthW(buffer, size, &len);
  if (FAILED(hr)) return false;
  return IsTextUnicode(buffer, static_cast<int>(len), nullptr);
}

template <typename T>
void PrintWinDbgFormat(T first, T last, Stat MtStat::*ptr,
                       IMtNameProvider *resolver) {
#ifdef _WIN64
  constexpr auto kHeader =
      "              MT    Count    TotalSize Class Name\n";
  constexpr auto kRowFormat = L"%016" PRIx64 "%9" PRIu64 "%13" PRIu64 " ";
#else
  constexpr auto kHeader = "      MT    Count    TotalSize Class Name\n";
  constexpr auto kRowFormat = L"%08" PRIx32 "%9" PRIu32 "%13" PRIu32 " ";
#endif
  printf(kHeader);
  wchar_t buffer[1024];
  size_t total_count = 0;
  size_t total_size = 0;
  for (auto it = first; it != last; ++it) {
    if (IsCancelled()) return;
    auto count = (*it.*ptr).count;
    auto size = (*it.*ptr).size_total;
    if (!count && !size) continue;
    wprintf(kRowFormat, it->addr, count, size);
    uint32_t needed;
    auto hr = resolver->GetMtName(it->addr, ARRAYSIZE(buffer), buffer, &needed);
    if (SUCCEEDED(hr)) {
      // Sometimes DAC returns garbage
      if (IsUnicodeString(buffer, ARRAYSIZE(buffer)))
        wprintf(L"%s\n", buffer);
      else
        wprintf(L"<error getting class name>\n");
    } else
      wprintf(L"<error getting class name, code 0x%08lx>\n", hr);
    total_count += count;
    total_size += size;
  }
  printf("Total %" PRIuPTR " objects\n", total_count);
  printf("Total size %" PRIuPTR " bytes\n", total_size);
}