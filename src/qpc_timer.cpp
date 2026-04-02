#include "qpc_timer.h"

namespace shh
{
std::uint32_t ComputeHighPrecisionTickCount(const HighPrecisionTimerState& state,
                                            std::uint64_t currentCounter)
{
    if (state.frequency == 0 || currentCounter <= state.startCounter)
    {
        return state.baseTickCount;
    }

    const std::uint64_t elapsedTicks = currentCounter - state.startCounter;
    const std::uint64_t elapsedMilliseconds = (elapsedTicks * 1000ULL) / state.frequency;
    return static_cast<std::uint32_t>(state.baseTickCount + elapsedMilliseconds);
}
} // namespace shh
