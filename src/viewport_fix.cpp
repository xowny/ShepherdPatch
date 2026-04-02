#include "viewport_fix.h"

#include <algorithm>
#include <cmath>

namespace shh
{
namespace
{
constexpr float kAspectEpsilon = 0.001f;
constexpr float kDefaultHudViewportAspect = 16.0f / 9.0f;
} // namespace

bool ClampHudViewport(D3DVIEWPORT9* viewport, float deviceWidth, float deviceHeight,
                      const Config& config)
{
    if (!config.enableHudViewportClamp || viewport == nullptr || viewport->Height == 0)
    {
        return false;
    }

    if (deviceWidth <= 0.0f || deviceHeight <= 0.0f)
    {
        return false;
    }

    if (viewport->Width >= deviceWidth - kAspectEpsilon &&
        viewport->Height >= deviceHeight - kAspectEpsilon)
    {
        return false;
    }

    const float targetAspect = config.hudViewportAspectRatio > kAspectEpsilon
                                   ? config.hudViewportAspectRatio
                                   : kDefaultHudViewportAspect;
    const float currentAspect =
        static_cast<float>(viewport->Width) / static_cast<float>(viewport->Height);
    if (currentAspect <= targetAspect + kAspectEpsilon)
    {
        return false;
    }

    const float desiredWidthF = viewport->Height * targetAspect;
    if (desiredWidthF < kAspectEpsilon)
    {
        return false;
    }

    const DWORD desiredWidth =
        static_cast<DWORD>(std::max(1.0f, std::round(desiredWidthF)));
    if (desiredWidth >= viewport->Width)
    {
        return false;
    }

    const DWORD delta = viewport->Width - desiredWidth;
    viewport->X += delta / 2;
    viewport->Width = desiredWidth;
    return true;
}
} // namespace shh
