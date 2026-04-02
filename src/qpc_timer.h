#pragma once

#include <cstdint>

namespace shh
{
struct HighPrecisionTimerState
{
    std::uint64_t frequency = 0;
    std::uint64_t startCounter = 0;
    std::uint32_t baseTickCount = 0;
};

std::uint32_t ComputeHighPrecisionTickCount(const HighPrecisionTimerState& state,
                                            std::uint64_t currentCounter);
} // namespace shh
