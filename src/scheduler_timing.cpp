#include "scheduler_timing.h"

#include "frame_rate_override.h"

#include <algorithm>

namespace shh
{
namespace
{
constexpr float kMillisecondsPerSecond = 1000.0f;

float ResolveEffectiveElapsedMilliseconds(float measuredWorkElapsedMilliseconds,
                                          float originalUpdateDeltaSeconds,
                                          float originalSleepMilliseconds)
{
    if (measuredWorkElapsedMilliseconds > 0.0f)
    {
        return measuredWorkElapsedMilliseconds;
    }

    if (originalUpdateDeltaSeconds > 0.0f)
    {
        return originalUpdateDeltaSeconds * kMillisecondsPerSecond;
    }

    return std::max(originalSleepMilliseconds, 0.0f);
}
} // namespace

SchedulerFramePlan BuildSchedulerFramePlan(bool frameRateUnlockEnabled,
                                           std::uint32_t targetFrameRate,
                                           float presentWakeLeadMilliseconds,
                                           float measuredWorkElapsedMilliseconds,
                                           float originalUpdateDeltaSeconds,
                                           float originalSleepMilliseconds)
{
    if (!frameRateUnlockEnabled)
    {
        return SchedulerFramePlan{
            .updateDeltaSeconds = originalUpdateDeltaSeconds,
            .sleepMilliseconds = std::max(originalSleepMilliseconds, 0.0f),
        };
    }

    if (targetFrameRate == 0)
    {
        const float effectiveElapsedMilliseconds =
            ResolveEffectiveElapsedMilliseconds(measuredWorkElapsedMilliseconds,
                                                originalUpdateDeltaSeconds,
                                                originalSleepMilliseconds);
        return SchedulerFramePlan{
            .updateDeltaSeconds = effectiveElapsedMilliseconds / kMillisecondsPerSecond,
            .sleepMilliseconds = 0.0f,
        };
    }

    const float targetFrameMilliseconds =
        ComputeFrameDurationMilliseconds(targetFrameRate);
    const float wakeLeadMilliseconds =
        std::max(presentWakeLeadMilliseconds, 0.0f);
    const float effectiveElapsedMilliseconds =
        measuredWorkElapsedMilliseconds > 0.0f ? measuredWorkElapsedMilliseconds
                                               : targetFrameMilliseconds;
    const float updateMilliseconds =
        std::max(effectiveElapsedMilliseconds, targetFrameMilliseconds);
    const float sleepBudgetMilliseconds =
        std::max(targetFrameMilliseconds - effectiveElapsedMilliseconds, 0.0f);
    const float sleepMilliseconds =
        std::max(sleepBudgetMilliseconds - wakeLeadMilliseconds, 0.0f);

    return SchedulerFramePlan{
        .updateDeltaSeconds = updateMilliseconds / kMillisecondsPerSecond,
        .sleepMilliseconds = sleepMilliseconds,
    };
}
} // namespace shh
