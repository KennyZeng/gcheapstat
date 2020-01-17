#include "mtstat_calculator.h"

MtStatCalculator::MtStatCalculator()
    : hprocess_{nullptr},
      sos_dac_interface_{nullptr},
      info_{},
      useful_globals_{} {}

HRESULT MtStatCalculator::Initialize(HANDLE hprocess,
                                     ISOSDacInterface* sos_dac_interface) {
  hprocess_ = hprocess;
  sos_dac_interface_ = sos_dac_interface;
  auto hr = info_.Request(sos_dac_interface_.get());
  hr = sos_dac_interface_->GetUsefulGlobals(&useful_globals_);
  // Heaps
  if (info_.bServerMode) {
    std::vector<CLRDATA_ADDRESS> heap_addr_list(info_.HeapCount);
    unsigned int needed = 0;
    hr = sos_dac_interface_->GetGCHeapList(info_.HeapCount, &heap_addr_list[0],
                                           &needed);
    if (FAILED(hr)) {
      LogError(L"Error getting GCHeapList\n");
      return hr;
    }
    HRESULT lasterror = S_OK;
    heaps_.reserve(info_.HeapCount);
    for (auto addr : heap_addr_list) {
      DacpGcHeapDetails heap{};
      hr = heap.Request(sos_dac_interface_.get(), addr);
      if (FAILED(hr)) {
        LogError(L"Error getting GcHeapDetails at 0x%016" PRIX64
                 ", code 0x%08lx\n",
                 addr, hr);
        lasterror = hr;
      } else
        heaps_.push_back(heap);
    }
    if (heaps_.empty()) return lasterror;
  } else {
    heaps_.resize(1);
    DacpGcHeapDetails heap{};
    hr = heap.Request(sos_dac_interface_.get());
    if (FAILED(hr)) {
      LogError(L"Error getting GcHeapDetails, code 0x%08lx\n", hr);
      return hr;
    } else
      heaps_.push_back(heap);
  }
  // Segments & gen0 allocation contexts
  for (size_t i = 0; i < heaps_.size(); ++i) {
    auto heap = &heaps_[i];
    if (heap->generation_table[0].allocContextPtr) {
      allocation_contexts_.push_back(
          {static_cast<uintptr_t>(heap->generation_table[0].allocContextPtr),
           static_cast<uintptr_t>(
               heap->generation_table[0].allocContextLimit)});
    }
    // Small & ephemeral
    auto addr = heap->generation_table[info_.g_max_generation].start_segment;
    for (; addr;) {
      Segment segment{static_cast<uintptr_t>(addr), heap};
      hr = segment.data.Request(sos_dac_interface_.get(), addr, *heap);
      if (FAILED(hr)) {
        LogError(L"Error getting SegmentData at 0x%016" PRIX64 "\n", addr);
        break;
      }
      segments_[0].push_back(segment);
      addr = segment.data.next;
    }
    // Large
    addr = heap->generation_table[info_.g_max_generation + 1].start_segment;
    for (; addr;) {
      Segment segment{static_cast<uintptr_t>(addr), heap};
      hr = segment.data.Request(sos_dac_interface_.get(), addr, *heap);
      if (FAILED(hr)) {
        LogError(L"Error getting SegmentData at 0x%016" PRIX64 "\n", addr);
        break;
      }
      segments_[1].push_back(segment);
      addr = segment.data.next;
    }
  }
  //  Thread allocation contexts
  DacpThreadStoreData threadstore_data{};
  hr = threadstore_data.Request(sos_dac_interface_.get());
  if (SUCCEEDED(hr)) {
    DacpThreadData thread_data{};
    for (auto thread = threadstore_data.firstThread; thread;
         thread = thread_data.nextThread) {
      hr = thread_data.Request(sos_dac_interface_.get(), thread);
      if (FAILED(hr)) {
        LogError(L"Error getting ThreadData at 0x%016" PRIX64
                 ", code 0x%08lx\n",
                 thread, hr);
      } else if (thread_data.allocContextPtr) {
        auto i = 0u;
        for (; i < allocation_contexts_.size() &&
               allocation_contexts_[i].ptr != thread_data.allocContextPtr;
             ++i)
          ;
        if (i == allocation_contexts_.size())
          allocation_contexts_.push_back(
              {static_cast<uintptr_t>(thread_data.allocContextPtr),
               static_cast<uintptr_t>(thread_data.allocContextLimit)});
      }
    }
  }
  for (auto it = allocation_contexts_.begin();
       it != allocation_contexts_.end();) {
    if (it->ptr < it->limit) {
      ++it;
      continue;
    }
    if (it->ptr == it->limit)
      LogError(L"Empty allocation context encountered\n");
    else
      LogError(L"Invalid allocation context encountered\n");
    it = allocation_contexts_.erase(it);
  }
  std::sort(allocation_contexts_.begin(), allocation_contexts_.end(),
            [](auto& a, auto& b) { return a.ptr < b.ptr; });
  return hr;
}

HRESULT MtStatCalculator::Calculate(std::vector<MtStat>& mtstat) {
  std::unordered_map<uintptr_t, MtAddrStat>{}.swap(dict_);
  for (auto& segment : segments_[0]) {
    if (segment.addr == segment.heap->ephemeral_heap_segment)
      WalkSegment<kAlignment>(segment.data.mem, segment.heap->alloc_allocated,
                              L"ephemeral");
    else
      WalkSegment<kAlignment>(segment.data.mem, segment.data.allocated,
                              L"small object");
    if (IsCancelled()) return S_FALSE;
  }
  for (auto& segment : segments_[1]) {
    WalkSegment<kAlignmentLarge>(segment.data.mem, segment.data.allocated,
                                 L"large object");
    if (IsCancelled()) return S_FALSE;
  }
  std::vector<MtStat> ret;
  ret.reserve(dict_.size());
  std::transform(dict_.cbegin(), dict_.cend(), std::back_inserter(ret),
                 [](auto& p) {
                   MtStat item;
                   item.addr = p.first;
                   item.count = p.second.count;
                   item.size_total = p.second.size_total;
                   return item;
                 });
  std::swap(ret, mtstat);
  return S_OK;
}

HRESULT CalculateMtStat(HANDLE hprocess, ISOSDacInterface* sos_dac_interface,
                        std::vector<MtStat>& mtstat) {
  MtStatCalculator impl;
  auto hr = impl.Initialize(hprocess, sos_dac_interface);
  if (SUCCEEDED(hr)) hr = impl.Calculate(mtstat);
  return hr;
}