#include "gameplay_delta.h"

#include "frame_rate_override.h"

#include <algorithm>

namespace shh
{
GameplayDeltaPlan BuildGameplayDeltaPlan(bool frameRateUnlockEnabled,
                                         std::uint32_t targetFrameRate,
                                         float measuredElapsedSeconds,
                                         float originalDeltaSeconds)
{
    if (!frameRateUnlockEnabled)
    {
        return GameplayDeltaPlan{.effectiveDeltaSeconds = originalDeltaSeconds};
    }

    if (targetFrameRate == 0)
    {
        const float effectiveDeltaSeconds =
            measuredElapsedSeconds > 0.0f ? measuredElapsedSeconds : originalDeltaSeconds;
        return GameplayDeltaPlan{.effectiveDeltaSeconds =
                                     std::max(effectiveDeltaSeconds, 0.0f)};
    }

    const float targetDeltaSeconds =
        ComputeFrameDurationMilliseconds(targetFrameRate) / 1000.0f;
    const float effectiveDeltaSeconds =
        measuredElapsedSeconds > 0.0f ? std::max(measuredElapsedSeconds, targetDeltaSeconds)
                                      : targetDeltaSeconds;
    return GameplayDeltaPlan{.effectiveDeltaSeconds = effectiveDeltaSeconds};
}
} // namespace shh
