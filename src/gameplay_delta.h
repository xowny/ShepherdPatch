#pragma once

#include <cstdint>

namespace shh
{
struct GameplayDeltaPlan
{
    float effectiveDeltaSeconds = 0.0f;
};

GameplayDeltaPlan BuildGameplayDeltaPlan(bool frameRateUnlockEnabled,
                                         std::uint32_t targetFrameRate,
                                         float measuredElapsedSeconds,
                                         float originalDeltaSeconds);
} // namespace shh
