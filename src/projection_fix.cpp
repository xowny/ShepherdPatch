#include "projection_fix.h"

#include <algorithm>
#include <cmath>

namespace shh
{
namespace
{
constexpr float kEpsilon = 0.0001f;

bool IsPerspectiveProjection(const ProjectionMatrix& matrix)
{
    return std::fabs(matrix.m34 - 1.0f) < kEpsilon && std::fabs(matrix.m44) < kEpsilon &&
           matrix.m11 > kEpsilon && matrix.m22 > kEpsilon;
}
} // namespace

bool AdjustProjectionMatrixForViewport(ProjectionMatrix* matrix, float viewportAspect,
                                       const Config& config)
{
    if (matrix == nullptr || viewportAspect <= kEpsilon || !IsPerspectiveProjection(*matrix))
    {
        return false;
    }

    const float originalAspect = matrix->m22 / matrix->m11;
    float targetAspect = originalAspect;

    if (config.enableUltrawideFovFix && viewportAspect > originalAspect + kEpsilon)
    {
        targetAspect = viewportAspect;
    }

    if (std::fabs(targetAspect - originalAspect) < kEpsilon)
    {
        return false;
    }

    matrix->m11 = matrix->m22 / targetAspect;
    return true;
}
} // namespace shh
