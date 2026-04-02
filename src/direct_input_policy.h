#pragma once

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

namespace shh
{
struct DirectInputMouseStateRecoveryPlan
{
    bool overrideResult = false;
    bool zeroState = false;
    HRESULT result = DI_OK;
};

DWORD SanitizeMouseCooperativeFlags(bool hardenDirectInputMouseDevice, bool isMouseDevice,
                                    DWORD requestedFlags);

DirectInputMouseStateRecoveryPlan BuildMouseStateRecoveryPlan(
    bool hardenDirectInputMouseDevice, bool isMouseDevice, HRESULT originalResult,
    bool hasMouseStateBuffer);
} // namespace shh
