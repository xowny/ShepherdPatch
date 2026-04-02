#pragma once

#include <cstdint>

namespace shh
{
struct LegacyPeriodicTimerPlan
{
    bool suppressTimer = false;
    std::uint32_t adjustedDelayMs = 0;
};

struct LegacyWaitableTimerWorkerPlan
{
    bool useTrackedTimer = false;
    std::uint32_t adjustedSleepMs = 0;
};

struct LegacyThreadControlPlan
{
    bool suppressOperation = false;
};

struct LegacyThreadTerminatePlan
{
    bool skipTerminateCall = false;
    std::uint32_t graceWaitMs = 0;
};

LegacyPeriodicTimerPlan BuildLegacyPeriodicTimerPlan(bool frameRateUnlockEnabled,
                                                     bool disableLegacyPeriodicIconicTimer,
                                                     std::uint32_t targetFrameRate,
                                                     std::uint32_t requestedDelayMs,
                                                     bool isLegacyCaller,
                                                     bool callbackMatchesExpected);

LegacyWaitableTimerWorkerPlan BuildLegacyWaitableTimerWorkerPlan(
    bool reduceLegacyWaitableTimerPolling, std::uint32_t requestedDelayMs,
    bool isLegacyWaitableTimerWorkerCaller, bool hasTrackedTimer);

LegacyThreadControlPlan BuildLegacyThreadControlPlan(bool hardenLegacyThreadWrapper,
                                                     bool isLegacyWrapperCaller,
                                                     bool threadAlreadyExited,
                                                     bool targetsCurrentThread);

LegacyThreadTerminatePlan BuildLegacyThreadTerminatePlan(bool hardenLegacyThreadWrapper,
                                                         std::uint32_t configuredGraceWaitMs,
                                                         bool isLegacyTerminateCaller,
                                                         bool threadAlreadyExited,
                                                         bool targetsCurrentThread);

std::uint32_t ResolveLegacyGraphicsRetryDelayMs(bool hardenLegacyGraphicsRecovery,
                                                std::uint32_t configuredRetryDelayMs,
                                                std::uint32_t requestedDelayMs);

bool ShouldSwallowSyntheticLegacyTimerKill(bool disableLegacyPeriodicIconicTimer,
                                           std::uint32_t timerId,
                                           std::uint32_t syntheticTimerId);
} // namespace shh
