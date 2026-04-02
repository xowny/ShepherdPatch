#include "raw_mouse.h"

#include <cmath>

namespace shh
{
void TransformRawMouseDelta(float sensitivity, bool invertY, long* deltaX, long* deltaY)
{
    if (deltaX == nullptr || deltaY == nullptr)
    {
        return;
    }

    const float clampedSensitivity = sensitivity < 0.0f ? 0.0f : sensitivity;
    *deltaX = static_cast<long>(std::lround(static_cast<double>(*deltaX) * clampedSensitivity));

    const long adjustedY =
        static_cast<long>(std::lround(static_cast<double>(*deltaY) * clampedSensitivity));
    *deltaY = invertY ? -adjustedY : adjustedY;
}

bool ApplyRawMouseDelta(void* state, unsigned long stateSize, long deltaX, long deltaY)
{
    if (state == nullptr)
    {
        return false;
    }

    if (stateSize == sizeof(DIMOUSESTATE))
    {
        auto* mouseState = static_cast<DIMOUSESTATE*>(state);
        mouseState->lX = deltaX;
        mouseState->lY = deltaY;
        return true;
    }

    if (stateSize == sizeof(DIMOUSESTATE2))
    {
        auto* mouseState = static_cast<DIMOUSESTATE2*>(state);
        mouseState->lX = deltaX;
        mouseState->lY = deltaY;
        return true;
    }

    return false;
}
} // namespace shh
