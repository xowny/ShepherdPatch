#include "projection_fix.h"
#include "test_registry.h"

#include <cmath>

namespace
{
constexpr float kPi = 3.14159265358979323846f;

float DegreesToRadians(float degrees)
{
    return degrees * kPi / 180.0f;
}

shh::ProjectionMatrix MakePerspective(float verticalFovDegrees, float aspectRatio)
{
    const float yScale = 1.0f / std::tan(DegreesToRadians(verticalFovDegrees) * 0.5f);

    shh::ProjectionMatrix matrix{};
    matrix.m11 = yScale / aspectRatio;
    matrix.m22 = yScale;
    matrix.m33 = 1.001001f;
    matrix.m34 = 1.0f;
    matrix.m43 = -0.1001001f;
    return matrix;
}

float ComputeAspect(const shh::ProjectionMatrix& matrix)
{
    return matrix.m22 / matrix.m11;
}
} // namespace

TEST_CASE(AdjustProjectionMatrixWidensPerspectiveForUltrawide)
{
    shh::Config config;
    config.enableUltrawideFovFix = true;

    shh::ProjectionMatrix matrix = MakePerspective(60.0f, 16.0f / 9.0f);

    const bool changed = shh::AdjustProjectionMatrixForViewport(&matrix, 21.0f / 9.0f, config);

    CHECK_TRUE(changed);
    CHECK_EQ(matrix.m22, MakePerspective(60.0f, 16.0f / 9.0f).m22);
    CHECK_EQ(std::round(ComputeAspect(matrix) * 1000.0f) / 1000.0f,
             std::round((21.0f / 9.0f) * 1000.0f) / 1000.0f);
}

TEST_CASE(AdjustProjectionMatrixSkipsOrthographicMatrices)
{
    shh::Config config;
    config.enableUltrawideFovFix = true;

    shh::ProjectionMatrix matrix{};
    matrix.m11 = 1.0f;
    matrix.m22 = 1.0f;
    matrix.m33 = 1.0f;
    matrix.m44 = 1.0f;

    const bool changed =
        shh::AdjustProjectionMatrixForViewport(&matrix, 21.0f / 9.0f, config);

    CHECK_TRUE(!changed);
    CHECK_EQ(matrix.m11, 1.0f);
    CHECK_EQ(matrix.m22, 1.0f);
}
