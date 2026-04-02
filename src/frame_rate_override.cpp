#include "frame_rate_override.h"

namespace shh
{
namespace
{
constexpr std::uint32_t kEffectivelyUncappedFrameRate = 1000;
constexpr std::uint32_t kLegacyPeriodicTimerDelayMs = 33;
constexpr std::uint32_t kMinimumSuspiciousFrameDelayMs = 15;
constexpr std::uint32_t kMaximumSuspiciousFrameDelayMs = 40;
}

std::uint32_t ResolveEngineFrameRate(bool frameRateUnlockEnabled,
                                     std::uint32_t targetFrameRate,
                                     std::uint32_t requestedFrameRate)
{
    if (!frameRateUnlockEnabled)
    {
        return requestedFrameRate;
    }

    if (targetFrameRate == 0)
    {
        return kEffectivelyUncappedFrameRate;
    }

    return targetFrameRate;
}

float ComputeFrameDurationMilliseconds(std::uint32_t frameRate)
{
    if (frameRate == 0)
    {
        return 0.0f;
    }

    return 1000.0f / static_cast<float>(frameRate);
}

std::uint32_t ResolveLegacyPeriodicTimerDelayMs(bool frameRateUnlockEnabled,
                                                std::uint32_t targetFrameRate,
                                                std::uint32_t requestedDelayMs)
{
    if (!frameRateUnlockEnabled)
    {
        return requestedDelayMs;
    }

    if (requestedDelayMs != kLegacyPeriodicTimerDelayMs)
    {
        return requestedDelayMs;
    }

    const std::uint32_t effectiveFrameRate =
        ResolveEngineFrameRate(frameRateUnlockEnabled, targetFrameRate, 30);
    const std::uint32_t targetDelayMs = 1000u / effectiveFrameRate;
    return targetDelayMs == 0 ? 1u : targetDelayMs;
}

std::uint32_t ResolveThirdPartyFramePacingDelayMs(bool frameRateUnlockEnabled,
                                                  std::uint32_t targetFrameRate,
                                                  std::uint32_t requestedDelayMs)
{
    if (!frameRateUnlockEnabled)
    {
        return requestedDelayMs;
    }

    if (requestedDelayMs < kMinimumSuspiciousFrameDelayMs ||
        requestedDelayMs > kMaximumSuspiciousFrameDelayMs)
    {
        return requestedDelayMs;
    }

    if (targetFrameRate == 0)
    {
        return 0;
    }

    const std::uint32_t targetDelayMs =
        static_cast<std::uint32_t>(ComputeFrameDurationMilliseconds(targetFrameRate));
    return targetDelayMs == 0 ? 1u : targetDelayMs;
}
} // namespace shh
