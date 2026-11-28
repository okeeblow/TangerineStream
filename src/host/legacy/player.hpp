#pragma once

#include "host/host_profile.hpp"
#include "core/model.hpp"
#include <Winamp/in2.h>

namespace tangerine {
std::wstring fromHostText(const char* text);
std::wstring fromHostFilename(const char* filename);
std::string toHostText(const wchar_t* text);
// Unlike display text, a filename must survive conversion without substitution.
std::string toHostFilename(const wchar_t* filename);

InputExtensions legacyInputExtensions(HWND player);
In_Module* selectLegacyInput(HWND player, const wchar_t* filename, const In_Module* exclude);
void copyLegacyHostCallbacks(In_Module& target, const In_Module& source);
}
