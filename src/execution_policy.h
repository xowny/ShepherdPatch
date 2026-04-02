#pragma once

#include <windows.h>

namespace shh
{
struct ModernExecutionPlan
{
    bool applyProcessPowerThrottling = false;
    ULONG processVersion = 0;
    ULONG processControlMask = 0;
    ULONG processStateMask = 0;

    bool applyThreadPowerThrottling = false;
    ULONG threadVersion = 0;
    ULONG threadControlMask = 0;
    ULONG threadStateMask = 0;

    bool applyGamesMmcssProfile = false;
};

ModernExecutionPlan BuildModernExecutionPlan(bool disablePowerThrottling,
                                             bool enableGamesMmcssProfile);
} // namespace shh
