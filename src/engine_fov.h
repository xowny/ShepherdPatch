#pragma once

#include "config.h"

namespace shh
{
bool AdjustEngineFovDegrees(float* currentFovDegrees, float* defaultFovDegrees, float displayAspect,
                            const Config& config);
} // namespace shh
