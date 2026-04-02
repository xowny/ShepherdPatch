#pragma once

#include "config.h"

#include <d3d9.h>

#include <cstdint>

namespace shh
{
bool ClampHudViewport(D3DVIEWPORT9* viewport, float deviceWidth, float deviceHeight,
                      const Config& config);
} // namespace shh
