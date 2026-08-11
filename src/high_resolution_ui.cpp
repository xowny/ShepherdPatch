#include "high_resolution_ui.h"

#include <array>

namespace shh
{
namespace
{
struct PanelResolutionGroup
{
    std::uint32_t maxWidth;
    std::uint32_t maxHeight;
    float scale;
    float offsetX;
    float extentX;
};

// The game uses discrete layouts for its textured map and document panels.
// Text is laid out separately and already scales correctly.
constexpr std::array<PanelResolutionGroup, 20> kPanelGroups{{
    {640, 480, 0.50f, 640.0f, 490.0f},
    {720, 576, 0.50f, 340.0f, 560.0f},
    {800, 600, 0.75f, 380.0f, 580.0f},
    {1024, 768, 0.75f, 490.0f, 790.0f},
    {1150, 864, 0.75f, 550.0f, 910.0f},
    {1280, 1024, 0.75f, 620.0f, 1030.0f},
    {1360, 768, 0.75f, 660.0f, 1100.0f},
    {1366, 768, 0.75f, 660.0f, 1108.0f},
    {1400, 1050, 0.75f, 670.0f, 1140.0f},
    {1440, 900, 0.75f, 690.0f, 1180.0f},
    {1600, 1200, 0.75f, 770.0f, 1330.0f},
    {1680, 1050, 0.75f, 810.0f, 1400.0f},
    {1920, 1080, 0.75f, 930.0f, 1630.0f},
    {2103, 1183, 1.00f, 1010.0f, 1750.0f},
    {2350, 1323, 1.00f, 1130.0f, 1980.0f},
    {2560, 1440, 1.00f, 1240.0f, 2170.0f},
    {2715, 1527, 1.25f, 1300.0f, 2270.0f},
    {2880, 1620, 1.25f, 1390.0f, 2420.0f},
    {3325, 1871, 1.50f, 1600.0f, 2780.0f},
    {3840, 2160, 1.50f, 1850.0f, 3260.0f},
}};

double ResolveItemScale(std::uint32_t width, std::uint32_t height)
{
    if (width <= 640 && height <= 480)
        return 0.0015812500116415321;
    if (width <= 800 && height <= 600)
        return 0.0012812500116415322;
    if (width <= 1024 && height <= 768)
        return 0.0009812500116415323;
    if (width <= 1366 && height <= 1024)
        return 0.0007812500116415322;
    if (width <= 1440 && height <= 1050)
        return 0.0007212500116415322;
    if (width <= 1680 && height <= 1050)
        return 0.0006412500116415322;
    if (width <= 1920 && height <= 1080)
        return 0.0005312500116415322;
    if (width <= 2560 && height <= 1440)
        return 0.0004062500116415322;
    return 0.0002812500116415322;
}
} // namespace

HighResolutionUiConstants ResolveHighResolutionUiConstants(
    std::uint32_t width, std::uint32_t height)
{
    // Keep stock-safe values until Direct3D reports a real backbuffer.
    if (width == 0 || height == 0)
    {
        return {0.75f, 670.0f, 940.0f, 0.0007812500116415322};
    }

    for (const auto& group : kPanelGroups)
    {
        if (width <= group.maxWidth && height <= group.maxHeight)
        {
            return {group.scale, group.offsetX, group.extentX,
                    ResolveItemScale(width, height)};
        }
    }

    // Preserve the largest known layout for uncommon resolutions above 4K.
    const auto& group = kPanelGroups.back();
    return {group.scale, group.offsetX, group.extentX,
            ResolveItemScale(width, height)};
}
} // namespace shh
