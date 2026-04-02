#pragma once

#include <cstdint>

namespace shh
{
struct Rect
{
    long left = 0;
    long top = 0;
    long right = 0;
    long bottom = 0;
};

struct BorderlessWindowPlan
{
    bool apply = false;
    long x = 0;
    long y = 0;
    long width = 0;
    long height = 0;
    std::uint32_t style = 0;
    std::uint32_t exStyle = 0;
};

BorderlessWindowPlan BuildBorderlessWindowPlan(bool forceBorderless, const Rect& monitorRect);
} // namespace shh
