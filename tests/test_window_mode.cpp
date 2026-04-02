#include "test_registry.h"
#include "window_mode.h"

#include <windows.h>

TEST_CASE(BuildBorderlessPlanUsesMonitorBounds)
{
    const shh::Rect monitor{100, 50, 2020, 1130};

    const shh::BorderlessWindowPlan plan = shh::BuildBorderlessWindowPlan(true, monitor);

    CHECK_TRUE(plan.apply);
    CHECK_EQ(plan.x, 100L);
    CHECK_EQ(plan.y, 50L);
    CHECK_EQ(plan.width, 1920L);
    CHECK_EQ(plan.height, 1080L);
    CHECK_EQ(plan.style, static_cast<unsigned long>(WS_POPUP));
    CHECK_EQ(plan.exStyle, static_cast<unsigned long>(WS_EX_APPWINDOW));
}

TEST_CASE(BuildBorderlessPlanSkipsWhenDisabled)
{
    const shh::Rect monitor{0, 0, 1920, 1080};

    const shh::BorderlessWindowPlan plan = shh::BuildBorderlessWindowPlan(false, monitor);

    CHECK_TRUE(!plan.apply);
    CHECK_EQ(plan.width, 0L);
    CHECK_EQ(plan.height, 0L);
}
