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

TEST_CASE(HighResolutionUiUsesGroundTruth640By480PanelConstants)
{
    const auto values = shh::ResolveHighResolutionUiConstants(640, 480);
    CHECK_TRUE(NearlyEqual(values.panelScale, 0.5));
    CHECK_TRUE(NearlyEqual(values.panelOffsetX, 300.0));
    CHECK_TRUE(NearlyEqual(values.panelExtentX, 490.0));
    CHECK_TRUE(NearlyEqual(values.itemScale, 0.0015812500116415321));
}

TEST_CASE(HighResolutionUiUsesGroundTruth1152By864PanelConstants)
{
    const auto values = shh::ResolveHighResolutionUiConstants(1152, 864);
    CHECK_TRUE(NearlyEqual(values.panelScale, 0.75));
    CHECK_TRUE(NearlyEqual(values.panelOffsetX, 550.0));
    CHECK_TRUE(NearlyEqual(values.panelExtentX, 910.0));
    CHECK_TRUE(NearlyEqual(values.itemScale, 0.0007812500116415322));
}

TEST_CASE(HighResolutionUiUsesGroundTruth1280By720PanelConstants)
{
    const auto values = shh::ResolveHighResolutionUiConstants(1280, 720);
    CHECK_TRUE(NearlyEqual(values.panelScale, 0.75));
    CHECK_TRUE(NearlyEqual(values.panelOffsetX, 670.0));
    CHECK_TRUE(NearlyEqual(values.panelExtentX, 940.0));
    CHECK_TRUE(NearlyEqual(values.itemScale, 0.0007812500116415322));
}

TEST_CASE(HighResolutionUiKeepsPanelAndItemTiersTogether)
{
    const auto values = shh::ResolveHighResolutionUiConstants(1366, 1024);
    CHECK_TRUE(NearlyEqual(values.panelOffsetX, 670.0));
    CHECK_TRUE(NearlyEqual(values.itemScale, 0.0007212500116415322));
}

TEST_CASE(HighResolutionUiPreservesCommonIntermediateResolutionItemScale)
{
    const auto values = shh::ResolveHighResolutionUiConstants(1600, 900);
    CHECK_TRUE(NearlyEqual(values.panelOffsetX, 770.0));
    CHECK_TRUE(NearlyEqual(values.panelExtentX, 1330.0));
    CHECK_TRUE(NearlyEqual(values.itemScale, 0.0006412500116415322));
}

TEST_CASE(HighResolutionUiPreservesTallIntermediateResolutionItemScale)
{
    const auto values = shh::ResolveHighResolutionUiConstants(1440, 1024);
    CHECK_TRUE(NearlyEqual(values.panelOffsetX, 770.0));
    CHECK_TRUE(NearlyEqual(values.panelExtentX, 1330.0));
    CHECK_TRUE(NearlyEqual(values.itemScale, 0.0007212500116415322));
}

TEST_CASE(HighResolutionUiFallsBackToLargestGroupAboveFourK)
{
    const auto values = shh::ResolveHighResolutionUiConstants(5120, 2880);
    CHECK_TRUE(NearlyEqual(values.panelScale, 1.5));
    CHECK_TRUE(NearlyEqual(values.panelOffsetX, 1850.0));
    CHECK_TRUE(NearlyEqual(values.panelExtentX, 3260.0));
    CHECK_TRUE(NearlyEqual(values.itemScale, 0.0002812500116415322));
}
