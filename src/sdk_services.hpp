#pragma once

#include <windows.h>
#include <objbase.h>
#include <cstdint>

// The 5.55 header supplies a pre-stdint MSVC typedef for plain char. Modern
// MSVC defines int8_t as signed char. Isolate the obsolete typedef in that header.
#ifdef _MSC_VER
#define int8_t wasabi_legacy_int8
#include <bfc/platform/types.h>
#undef int8_t
#endif
#include <api/service/api_service.h>
