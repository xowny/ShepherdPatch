#include "engine_fov.h"
#include "test_registry.h"

#include <cmath>

namespace
{
constexpr float kPi = 3.14159265358979323846f;
constexpr float kBaseAspect = 16.0f / 9.0f;

float Round3(float value)
{
    return std::round(value * 1000.0f) / 1000.0f;
}
} // namespace

TEST_CASE(AdjustEngineFovDegreesWidensForUltrawide)
{
    shh::Config config;
    config.enableUltrawideFovFix = true;

    float currentFovDegrees = 60.0f;
    float defaultFovDegrees = 60.0f;

    const bool changed =
        shh::AdjustEngineFovDegrees(&currentFovDegrees, &defaultFovDegrees, 21.0f / 9.0f, config);

    CHECK_TRUE(changed);
    CHECK_TRUE(defaultFovDegrees > 60.0f);
    CHECK_EQ(Round3(currentFovDegrees), Round3(defaultFovDegrees));
}

TEST_CASE(AdjustEngineFovDegreesSkipsWhenUnchanged)
{
    shh::Config config;
    config.enableUltrawideFovFix = false;

    float currentFovDegrees = 60.0f;
    float defaultFovDegrees = 60.0f;

    const bool changed =
        shh::AdjustEngineFovDegrees(&currentFovDegrees, &defaultFovDegrees, kBaseAspect, config);

    CHECK_TRUE(!changed);
    CHECK_EQ(Round3(currentFovDegrees), Round3(60.0f));
    CHECK_EQ(Round3(defaultFovDegrees), Round3(60.0f));
}
