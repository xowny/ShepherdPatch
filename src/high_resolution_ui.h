#pragma once

#include <cstdint>

namespace shh
{
struct HighResolutionUiConstants
{
    float panelScale;
    float panelOffsetX;
    float panelExtentX;
    double itemScale;
};

HighResolutionUiConstants ResolveHighResolutionUiConstants(
    std::uint32_t width, std::uint32_t height);
} // namespace shh
