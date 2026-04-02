#include "precise_sleep.h"

namespace shh
{
namespace
{
constexpr std::uint32_t kMaxPreciseSleepMs = 16;
constexpr float kMinimumSpinWaitMs = 0.25f;
constexpr float kLegacyFrameCapFps = 30.0f;
constexpr float kMillisecondsPerSecond = 1000.0f;
}

PreciseSleepPlan BuildPreciseSleepPlan(bool enabled, float requestedMilliseconds,
                                       bool alertable)
{
    PreciseSleepPlan plan;
    if (!enabled || alertable || requestedMilliseconds <= 0.0f)
    {
        return plan;
    }

    const auto truncatedMilliseconds = static_cast<std::uint32_t>(requestedMilliseconds);
    if (truncatedMilliseconds == 0 || truncatedMilliseconds > kMaxPreciseSleepMs)
    {
        return plan;
    }

    plan.useOriginalSleep = false;
    plan.coarseSleepMs = truncatedMilliseconds > 1 ? truncatedMilliseconds - 1 : 0;
    plan.spinWaitMs = 1.0f;
    return plan;
}

PreciseSleepPlan BuildAlertablePreciseSleepPlan(bool enabled, float requestedMilliseconds)
{
    PreciseSleepPlan plan;
    if (!enabled || requestedMilliseconds <= 0.0f)
    {
        return plan;
    }

    if (requestedMilliseconds > static_cast<float>(kMaxPreciseSleepMs))
    {
        return plan;
    }

    plan.useOriginalSleep = false;
    if (requestedMilliseconds >= 1.0f)
    {
        const auto truncatedMilliseconds =
            static_cast<std::uint32_t>(requestedMilliseconds);
        plan.coarseSleepMs = truncatedMilliseconds > 1 ? truncatedMilliseconds - 1 : 0;
    }

    plan.spinWaitMs = requestedMilliseconds - static_cast<float>(plan.coarseSleepMs);
    if (plan.spinWaitMs < kMinimumSpinWaitMs)
    {
        plan.spinWaitMs = kMinimumSpinWaitMs;
    }
    return plan;
}

float AdjustLegacyFramePacingSleepMilliseconds(bool frameRateUnlockEnabled,
                                               std::uint32_t targetFrameRate,
                                               float requestedMilliseconds)
{
    if (!frameRateUnlockEnabled || targetFrameRate == 0 || targetFrameRate <= 30 ||
        requestedMilliseconds <= 0.0f)
    {
        return requestedMilliseconds;
    }

    const float legacyFrameDurationMs = kMillisecondsPerSecond / kLegacyFrameCapFps;
    const float targetFrameDurationMs =
        kMillisecondsPerSecond / static_cast<float>(targetFrameRate);
    const float deltaMs = legacyFrameDurationMs - targetFrameDurationMs;
    if (deltaMs <= 0.0f)
    {
        return requestedMilliseconds;
    }

    const float adjustedMilliseconds = requestedMilliseconds - deltaMs;
    return adjustedMilliseconds > 0.0f ? adjustedMilliseconds : 0.0f;
}
} // namespace shh
