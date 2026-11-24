#pragma once

#include "sdk_services.hpp"

// The SDK checks an undefined _MSC_VER as zero and supplies its own intptr_t.
#ifndef _MSC_VER
#define _MSC_VER 1900
#define RESTORE_MINGW_MACROS
#endif
#include <gen_ml/ml.h>
#include <gen_ml/ml_ipc_0313.h>
#include <Winamp/wa_ipc.h>
#ifdef RESTORE_MINGW_MACROS
#undef _MSC_VER
#undef RESTORE_MINGW_MACROS
#endif
