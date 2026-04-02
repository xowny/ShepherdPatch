#pragma once

#include <string_view>

namespace shh
{
enum class BinkTrackedFunction
{
    Wait,
    DoFrame,
    NextFrame,
    Pause,
};

bool ShouldTraceBinkPlayback(std::string_view moviePath);
std::string_view GetBinkTrackedFunctionName(BinkTrackedFunction function);
} // namespace shh
