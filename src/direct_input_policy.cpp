#include "direct_input_policy.h"

namespace shh
{
namespace
{
constexpr DWORD kRawMouseFriendlyCooperativeFlags = DISCL_FOREGROUND | DISCL_NONEXCLUSIVE;

bool IsRecoverableMousePollResult(HRESULT result)
{
    return result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED;
}
} // namespace

DWORD SanitizeMouseCooperativeFlags(bool hardenDirectInputMouseDevice, bool isMouseDevice,
                                    DWORD requestedFlags)
{
    if (!hardenDirectInputMouseDevice || !isMouseDevice)
    {
        return requestedFlags;
    }

    const DWORD conflictingFlags = DISCL_BACKGROUND | DISCL_EXCLUSIVE | DISCL_NOWINKEY;
    return (requestedFlags & ~conflictingFlags) | kRawMouseFriendlyCooperativeFlags;
}

DirectInputMouseStateRecoveryPlan BuildMouseStateRecoveryPlan(
    bool hardenDirectInputMouseDevice, bool isMouseDevice, HRESULT originalResult,
    bool hasMouseStateBuffer)
{
    const bool overrideResult = hardenDirectInputMouseDevice && isMouseDevice &&
                                hasMouseStateBuffer &&
                                IsRecoverableMousePollResult(originalResult);
    return DirectInputMouseStateRecoveryPlan{
        .overrideResult = overrideResult,
        .zeroState = overrideResult,
        .result = overrideResult ? static_cast<HRESULT>(DI_OK) : originalResult,
    };
}
} // namespace shh
