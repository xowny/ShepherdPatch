#include "engine_fov.h"

#include <algorithm>
#include <cmath>

namespace shh
{
namespace
{
constexpr float kEpsilon = 0.0001f;
constexpr float kPi = 3.14159265358979323846f;
constexpr float kBaseAspect = 16.0f / 9.0f;
constexpr float kMinFovDegrees = 40.0f;
constexpr float kMaxFovDegrees = 140.0f;

float DegreesToRadians(float degrees)
{
    return degrees * kPi / 180.0f;
}

float RadiansToDegrees(float radians)
{
    return radians * 180.0f / kPi;
}

float AdjustHorizontalFovForAspect(float horizontalFovDegrees, float targetAspect)
{
    const float tangent = std::tan(DegreesToRadians(horizontalFovDegrees) * 0.5f);
    return RadiansToDegrees(2.0f * std::atan(tangent * (targetAspect / kBaseAspect)));
}
} // namespace

bool AdjustEngineFovDegrees(float* currentFovDegrees, float* defaultFovDegrees, float displayAspect,
                            const Config& config)
{
    if (currentFovDegrees == nullptr || defaultFovDegrees == nullptr || displayAspect <= kEpsilon)
    {
        return false;
    }

    float targetFovDegrees = *defaultFovDegrees > kEpsilon ? *defaultFovDegrees : *currentFovDegrees;
    if (targetFovDegrees <= kEpsilon)
    {
        return false;
    }

    if (config.enableUltrawideFovFix && displayAspect > kBaseAspect + kEpsilon)
    {
        targetFovDegrees = AdjustHorizontalFovForAspect(targetFovDegrees, displayAspect);
    }

    targetFovDegrees = std::clamp(targetFovDegrees, kMinFovDegrees, kMaxFovDegrees);
    if (std::fabs(*currentFovDegrees - targetFovDegrees) < kEpsilon &&
        std::fabs(*defaultFovDegrees - targetFovDegrees) < kEpsilon)
    {
        return false;
    }

    *currentFovDegrees = targetFovDegrees;
    *defaultFovDegrees = targetFovDegrees;
    return true;
}
} // namespace shh
