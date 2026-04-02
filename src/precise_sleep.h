#pragma once

#include <cstdint>

namespace shh
{
struct PreciseSleepPlan
{
    bool useOriginalSleep = true;
    std::uint32_t coarseSleepMs = 0;
    float spinWaitMs = 0.0f;
};

PreciseSleepPlan BuildPreciseSleepPlan(bool enabled, float requestedMilliseconds,
                                       bool alertable);
PreciseSleepPlan BuildAlertablePreciseSleepPlan(bool enabled, float requestedMilliseconds);
float AdjustLegacyFramePacingSleepMilliseconds(bool frameRateUnlockEnabled,
                                               std::uint32_t targetFrameRate,
                                               float requestedMilliseconds);
} // namespace shh
