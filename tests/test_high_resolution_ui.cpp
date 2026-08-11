#include "high_resolution_ui.h"
#include "test_registry.h"

#include <cmath>

namespace
{
bool NearlyEqual(double actual, double expected, double epsilon = 1e-12)
{
    return std::abs(actual - expected) <= epsilon;
}
} // namespace

TEST_CASE(HighResolutionUiUsesStockSafeConstantsBeforeDeviceCreation)
{
    const auto values = shh::ResolveHighResolutionUiConstants(0, 0);
    CHECK_TRUE(NearlyEqual(values.panelScale, 0.75));
    CHECK_TRUE(NearlyEqual(values.panelOffsetX, 670.0));
    CHECK_TRUE(NearlyEqual(values.panelExtentX, 940.0));
    CHECK_TRUE(NearlyEqual(values.itemScale, 0.0007812500116415322));
}

TEST_CASE(HighResolutionUiUsesCorrect2560By1440PanelConstants)
{
    const auto values = shh::ResolveHighResolutionUiConstants(2560, 1440);
    CHECK_TRUE(NearlyEqual(values.panelScale, 1.0));
    CHECK_TRUE(NearlyEqual(values.panelOffsetX, 1240.0));
    CHECK_TRUE(NearlyEqual(values.panelExtentX, 2170.0));
    CHECK_TRUE(NearlyEqual(values.itemScale, 0.0004062500116415322));
}

TEST_CASE(HighResolutionUiSelectsGroupsUsingBothDimensions)
{
    const auto values = shh::ResolveHighResolutionUiConstants(1920, 1200);
    CHECK_TRUE(NearlyEqual(values.panelOffsetX, 1130.0));
    CHECK_TRUE(NearlyEqual(values.panelExtentX, 1980.0));
}
