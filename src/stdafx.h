#pragma once
#include <winsdkver.h>
//
#include <comdef.h>
#include <corhdr.h>
#include <pathcch.h>
#include <shlwapi.h>
#include <strsafe.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cinttypes>  // PRIX32
#include <memory>
#include <unordered_map>
#include <vector>

#include "cancellation.h"
#include "crosscomp.h"
#include "output.h"
#include "wil/com.h"
#include "wil/resource.h"