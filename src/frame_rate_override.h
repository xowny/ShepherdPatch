#pragma once

#include <cstdint>

namespace shh
{
std::uint32_t ResolveEngineFrameRate(bool frameRateUnlockEnabled,
                                     std::uint32_t targetFrameRate,
                                     std::uint32_t requestedFrameRate);

float ComputeFrameDurationMilliseconds(std::uint32_t frameRate);
std::uint32_t ResolveLegacyPeriodicTimerDelayMs(bool frameRateUnlockEnabled,
                                                std::uint32_t targetFrameRate,
                                                std::uint32_t requestedDelayMs);
std::uint32_t ResolveThirdPartyFramePacingDelayMs(bool frameRateUnlockEnabled,
                                                  std::uint32_t targetFrameRate,
                                                  std::uint32_t requestedDelayMs);
} // namespace shh
