#include "legacy_runtime_policy.h"

#include "frame_rate_override.h"

namespace shh
{
namespace
{
constexpr std::uint32_t kLegacyGraphicsRetryDelayMs = 5000;
}

LegacyPeriodicTimerPlan BuildLegacyPeriodicTimerPlan(bool frameRateUnlockEnabled,
                                                     bool disableLegacyPeriodicIconicTimer,
                                                     std::uint32_t targetFrameRate,
                                                     std::uint32_t requestedDelayMs,
                                                     bool isLegacyCaller,
                                                     bool callbackMatchesExpected)
{
    if (!isLegacyCaller)
    {
        return LegacyPeriodicTimerPlan{
            .suppressTimer = false,
            .adjustedDelayMs = requestedDelayMs,
        };
    }

    if (disableLegacyPeriodicIconicTimer && callbackMatchesExpected)
    {
        return LegacyPeriodicTimerPlan{
            .suppressTimer = true,
            .adjustedDelayMs = 0,
        };
    }

    return LegacyPeriodicTimerPlan{
        .suppressTimer = false,
        .adjustedDelayMs = ResolveLegacyPeriodicTimerDelayMs(frameRateUnlockEnabled,
                                                             targetFrameRate, requestedDelayMs),
    };
}

LegacyWaitableTimerWorkerPlan BuildLegacyWaitableTimerWorkerPlan(
    bool reduceLegacyWaitableTimerPolling, std::uint32_t requestedDelayMs,
    bool isLegacyWaitableTimerWorkerCaller, bool hasTrackedTimer)
{
    if (!reduceLegacyWaitableTimerPolling || !isLegacyWaitableTimerWorkerCaller ||
        requestedDelayMs != 1)
    {
        return LegacyWaitableTimerWorkerPlan{
            .useTrackedTimer = false,
            .adjustedSleepMs = requestedDelayMs,
        };
    }

    return LegacyWaitableTimerWorkerPlan{
        .useTrackedTimer = hasTrackedTimer,
        .adjustedSleepMs = hasTrackedTimer ? requestedDelayMs : 0u,
    };
}

LegacyThreadControlPlan BuildLegacyThreadControlPlan(bool hardenLegacyThreadWrapper,
                                                     bool isLegacyWrapperCaller,
                                                     bool threadAlreadyExited,
                                                     bool targetsCurrentThread)
{
    return LegacyThreadControlPlan{
        .suppressOperation = hardenLegacyThreadWrapper && isLegacyWrapperCaller &&
                             (threadAlreadyExited || targetsCurrentThread),
    };
}

LegacyThreadTerminatePlan BuildLegacyThreadTerminatePlan(bool hardenLegacyThreadWrapper,
                                                         std::uint32_t configuredGraceWaitMs,
                                                         bool isLegacyTerminateCaller,
                                                         bool threadAlreadyExited,
                                                         bool targetsCurrentThread)
{
    if (!hardenLegacyThreadWrapper || !isLegacyTerminateCaller)
    {
        return LegacyThreadTerminatePlan{
            .skipTerminateCall = false,
            .graceWaitMs = 0,
        };
    }

    if (threadAlreadyExited || targetsCurrentThread)
    {
        return LegacyThreadTerminatePlan{
            .skipTerminateCall = true,
            .graceWaitMs = 0,
        };
    }

    return LegacyThreadTerminatePlan{
        .skipTerminateCall = false,
        .graceWaitMs = configuredGraceWaitMs,
    };
}

std::uint32_t ResolveLegacyGraphicsRetryDelayMs(bool hardenLegacyGraphicsRecovery,
                                                std::uint32_t configuredRetryDelayMs,
                                                std::uint32_t requestedDelayMs)
{
    if (!hardenLegacyGraphicsRecovery || requestedDelayMs != kLegacyGraphicsRetryDelayMs)
    {
        return requestedDelayMs;
    }

    return configuredRetryDelayMs;
}

bool ShouldSwallowSyntheticLegacyTimerKill(bool disableLegacyPeriodicIconicTimer,
                                           std::uint32_t timerId,
                                           std::uint32_t syntheticTimerId)
{
    return disableLegacyPeriodicIconicTimer && syntheticTimerId != 0 &&
           timerId == syntheticTimerId;
}
} // namespace shh
