#include "precise_sleep.h"
#include "test_registry.h"

TEST_CASE(BuildPreciseSleepPlanSkipsDisabledOrAlertableSleeps)
{
    const shh::PreciseSleepPlan disabled = shh::BuildPreciseSleepPlan(false, 1.0f, false);
    CHECK_TRUE(disabled.useOriginalSleep);
    CHECK_EQ(disabled.coarseSleepMs, 0u);
    CHECK_EQ(disabled.spinWaitMs, 0.0f);

    const shh::PreciseSleepPlan alertable = shh::BuildPreciseSleepPlan(true, 1.0f, true);
    CHECK_TRUE(alertable.useOriginalSleep);
    CHECK_EQ(alertable.coarseSleepMs, 0u);
    CHECK_EQ(alertable.spinWaitMs, 0.0f);
}

TEST_CASE(BuildPreciseSleepPlanUsesHybridWaitForShortNonAlertableSleeps)
{
    const shh::PreciseSleepPlan oneMs = shh::BuildPreciseSleepPlan(true, 1.9f, false);
    CHECK_TRUE(!oneMs.useOriginalSleep);
    CHECK_EQ(oneMs.coarseSleepMs, 0u);
    CHECK_EQ(oneMs.spinWaitMs, 1.0f);

    const shh::PreciseSleepPlan eightMs = shh::BuildPreciseSleepPlan(true, 8.9f, false);
    CHECK_TRUE(!eightMs.useOriginalSleep);
    CHECK_EQ(eightMs.coarseSleepMs, 7u);
    CHECK_EQ(eightMs.spinWaitMs, 1.0f);
}

TEST_CASE(BuildPreciseSleepPlanLeavesLongOrSubMillisecondSleepsAlone)
{
    const shh::PreciseSleepPlan subMillisecond = shh::BuildPreciseSleepPlan(true, 0.9f, false);
    CHECK_TRUE(subMillisecond.useOriginalSleep);

    const shh::PreciseSleepPlan longSleep = shh::BuildPreciseSleepPlan(true, 50.0f, false);
    CHECK_TRUE(longSleep.useOriginalSleep);
}

TEST_CASE(BuildAlertablePreciseSleepPlanUsesCoarseAlertableSleepAndFractionalSpin)
{
    const shh::PreciseSleepPlan plan =
        shh::BuildAlertablePreciseSleepPlan(true, 15.35f);
    CHECK_TRUE(!plan.useOriginalSleep);
    CHECK_EQ(plan.coarseSleepMs, 14u);
    CHECK_TRUE(plan.spinWaitMs > 1.3f && plan.spinWaitMs < 1.4f);
}

TEST_CASE(BuildAlertablePreciseSleepPlanAllowsPureSpinForSubMillisecondTail)
{
    const shh::PreciseSleepPlan plan =
        shh::BuildAlertablePreciseSleepPlan(true, 0.6f);
    CHECK_TRUE(!plan.useOriginalSleep);
    CHECK_EQ(plan.coarseSleepMs, 0u);
    CHECK_TRUE(plan.spinWaitMs > 0.5f && plan.spinWaitMs < 0.7f);
}

TEST_CASE(BuildAlertablePreciseSleepPlanLeavesDisabledOrLongSleepsAlone)
{
    CHECK_TRUE(shh::BuildAlertablePreciseSleepPlan(false, 10.0f).useOriginalSleep);
    CHECK_TRUE(shh::BuildAlertablePreciseSleepPlan(true, 30.0f).useOriginalSleep);
}

TEST_CASE(AdjustLegacyFramePacingSleepMillisecondsLeavesRequestsAloneWhenUnlockDisabled)
{
    CHECK_EQ(shh::AdjustLegacyFramePacingSleepMilliseconds(false, 60, 15.0f), 15.0f);
}

TEST_CASE(AdjustLegacyFramePacingSleepMillisecondsRemovesLegacyThirtyFpsBudgetDelta)
{
    const float adjusted = shh::AdjustLegacyFramePacingSleepMilliseconds(true, 60, 15.6142f);
    CHECK_TRUE(adjusted >= 0.0f);
    CHECK_TRUE(adjusted < 0.001f);

    const float delayed = shh::AdjustLegacyFramePacingSleepMilliseconds(true, 60, 25.0f);
    CHECK_TRUE(delayed > 8.3f && delayed < 8.4f);
}
