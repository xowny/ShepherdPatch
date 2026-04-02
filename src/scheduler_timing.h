#pragma once

#include <cstdint>

namespace shh
{
struct SchedulerFramePlan
{
    float updateDeltaSeconds = 0.0f;
    float sleepMilliseconds = 0.0f;
};

SchedulerFramePlan BuildSchedulerFramePlan(bool frameRateUnlockEnabled,
                                           std::uint32_t targetFrameRate,
                                           float presentWakeLeadMilliseconds,
                                           float measuredWorkElapsedMilliseconds,
                                           float originalUpdateDeltaSeconds,
                                           float originalSleepMilliseconds);
} // namespace shh
