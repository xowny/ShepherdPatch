#pragma once

#include <cstdint>
#include <string_view>

namespace shh
{
struct MenuMovieWaitPlan
{
    bool trackHandle = false;
    bool forceZeroWait = false;
};
bool ShouldApplyMenuMovieWaitFix(std::string_view moviePath);
MenuMovieWaitPlan BuildMenuMovieWaitPlan(bool reduceMenuMovieStutter,
                                         std::string_view moviePath);
} // namespace shh
