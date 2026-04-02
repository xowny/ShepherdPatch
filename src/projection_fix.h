#pragma once

#include "config.h"

namespace shh
{
struct ProjectionMatrix
{
    float m11 = 0.0f;
    float m12 = 0.0f;
    float m13 = 0.0f;
    float m14 = 0.0f;
    float m21 = 0.0f;
    float m22 = 0.0f;
    float m23 = 0.0f;
    float m24 = 0.0f;
    float m31 = 0.0f;
    float m32 = 0.0f;
    float m33 = 0.0f;
    float m34 = 0.0f;
    float m41 = 0.0f;
    float m42 = 0.0f;
    float m43 = 0.0f;
    float m44 = 0.0f;
};

bool AdjustProjectionMatrixForViewport(ProjectionMatrix* matrix, float viewportAspect,
                                       const Config& config);
} // namespace shh
