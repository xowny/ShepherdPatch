#pragma once

#include <windows.h>

namespace shh
{
void OnProcessAttach(HMODULE module);
void OnProcessDetach();
} // namespace shh
