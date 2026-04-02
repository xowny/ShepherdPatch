#include "scheduler_timing.h"
#include "test_registry.h"

TEST_CASE(BuildSchedulerFramePlanLeavesOriginalTimingWhenUnlockDisabled)
{
    const shh::SchedulerFramePlan plan =
        shh::BuildSchedulerFramePlan(false, 60, 0.75f, 5.0f, 1.0f / 30.0f, 16.0f);

    CHECK_EQ(plan.updateDeltaSeconds, 1.0f / 30.0f);
    CHECK_EQ(plan.sleepMilliseconds, 16.0f);
}

TEST_CASE(BuildSchedulerFramePlanTargetsConfiguredFrameRateWhenWorkIsCheap)
{
    const shh::SchedulerFramePlan plan =
        shh::BuildSchedulerFramePlan(true, 60, 0.0f, 5.0f, 1.0f / 30.0f, 16.0f);

    CHECK_TRUE(plan.updateDeltaSeconds > 0.0166f && plan.updateDeltaSeconds < 0.0167f);
    CHECK_TRUE(plan.sleepMilliseconds > 11.6f && plan.sleepMilliseconds < 11.7f);
}

TEST_CASE(BuildSchedulerFramePlanPreservesRealTimeWhenWorkExceedsBudget)
{
    const shh::SchedulerFramePlan plan =
        shh::BuildSchedulerFramePlan(true, 60, 0.0f, 22.0f, 1.0f / 30.0f, 16.0f);

    CHECK_EQ(plan.sleepMilliseconds, 0.0f);
    CHECK_TRUE(plan.updateDeltaSeconds > 0.0219f && plan.updateDeltaSeconds < 0.0221f);
}

TEST_CASE(BuildSchedulerFramePlanRunsUnlockedWithoutArtificialSleep)
{
    const shh::SchedulerFramePlan plan =
        shh::BuildSchedulerFramePlan(true, 0, 0.75f, 6.0f, 1.0f / 30.0f, 16.0f);

    CHECK_EQ(plan.sleepMilliseconds, 0.0f);
    CHECK_TRUE(plan.updateDeltaSeconds > 0.0059f && plan.updateDeltaSeconds < 0.0061f);
}

TEST_CASE(BuildSchedulerFramePlanFallsBackToOriginalDeltaWhenNoMeasurementExists)
{
    const shh::SchedulerFramePlan plan =
        shh::BuildSchedulerFramePlan(true, 0, 0.75f, 0.0f, 1.0f / 30.0f, 16.0f);

    CHECK_EQ(plan.sleepMilliseconds, 0.0f);
    CHECK_EQ(plan.updateDeltaSeconds, 1.0f / 30.0f);
}

TEST_CASE(BuildSchedulerFramePlanUsesTargetCadenceWhenNoMeasurementExistsForFixedCap)
{
    const shh::SchedulerFramePlan plan =
        shh::BuildSchedulerFramePlan(true, 60, 0.75f, 0.0f, 1.0f / 30.0f, 16.0f);

    CHECK_EQ(plan.sleepMilliseconds, 0.0f);
    CHECK_TRUE(plan.updateDeltaSeconds > 0.0166f && plan.updateDeltaSeconds < 0.0167f);
}

TEST_CASE(BuildSchedulerFramePlanAppliesWakeLeadToFixedVsyncCap)
{
    const shh::SchedulerFramePlan plan =
        shh::BuildSchedulerFramePlan(true, 60, 0.75f, 5.0f, 1.0f / 30.0f, 16.0f);

    CHECK_TRUE(plan.updateDeltaSeconds > 0.0166f && plan.updateDeltaSeconds < 0.0167f);
    CHECK_TRUE(plan.sleepMilliseconds > 10.9f && plan.sleepMilliseconds < 11.0f);
}

TEST_CASE(BuildSchedulerFramePlanDoesNotUnderflowSleepWhenLeadExceedsBudget)
{
    const shh::SchedulerFramePlan plan =
        shh::BuildSchedulerFramePlan(true, 60, 1.0f, 16.2f, 16.0f, 16.0f);

    CHECK_EQ(plan.sleepMilliseconds, 0.0f);
    CHECK_TRUE(plan.updateDeltaSeconds > 0.0166f && plan.updateDeltaSeconds < 0.0167f);
}
