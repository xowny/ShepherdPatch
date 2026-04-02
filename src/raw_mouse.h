#pragma once

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

namespace shh
{
bool ApplyRawMouseDelta(void* state, unsigned long stateSize, long deltaX, long deltaY);
void TransformRawMouseDelta(float sensitivity, bool invertY, long* deltaX, long* deltaY);
} // namespace shh
