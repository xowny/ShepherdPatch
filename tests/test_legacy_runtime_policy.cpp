#include "legacy_runtime_policy.h"
#include "test_registry.h"

TEST_CASE(BuildLegacyPeriodicTimerPlanLeavesOrdinaryTimersAlone)
{
    const shh::LegacyPeriodicTimerPlan plan =
        shh::BuildLegacyPeriodicTimerPlan(true, true, 60, 33, false, true);

    CHECK_TRUE(!plan.suppressTimer);
    CHECK_EQ(plan.adjustedDelayMs, 33u);
}

TEST_CASE(BuildLegacyPeriodicTimerPlanSuppressesLegacyIconicTimerWhenEnabled)
{
    const shh::LegacyPeriodicTimerPlan plan =
        shh::BuildLegacyPeriodicTimerPlan(true, true, 60, 33, true, true);

    CHECK_TRUE(plan.suppressTimer);
    CHECK_EQ(plan.adjustedDelayMs, 0u);
}

TEST_CASE(BuildLegacyPeriodicTimerPlanRetimesLegacyTimerWhenSuppressionDisabled)
{
    const shh::LegacyPeriodicTimerPlan plan =
        shh::BuildLegacyPeriodicTimerPlan(true, false, 60, 33, true, true);

    CHECK_TRUE(!plan.suppressTimer);
    CHECK_EQ(plan.adjustedDelayMs, 16u);
}

TEST_CASE(ResolveLegacyGraphicsRetryDelayMsClampsFiveSecondRetryLoop)
{
    CHECK_EQ(shh::ResolveLegacyGraphicsRetryDelayMs(true, 100, 5000), 100u);
    CHECK_EQ(shh::ResolveLegacyGraphicsRetryDelayMs(true, 0, 5000), 0u);
}

TEST_CASE(ResolveLegacyGraphicsRetryDelayMsLeavesNormalSleepsAlone)
{
    CHECK_EQ(shh::ResolveLegacyGraphicsRetryDelayMs(false, 100, 5000), 5000u);
    CHECK_EQ(shh::ResolveLegacyGraphicsRetryDelayMs(true, 100, 33), 33u);
}

TEST_CASE(ShouldSwallowSyntheticLegacyTimerKillMatchesSyntheticIdOnly)
{
    CHECK_TRUE(shh::ShouldSwallowSyntheticLegacyTimerKill(true, 0x51510001u, 0x51510001u));
    CHECK_TRUE(!shh::ShouldSwallowSyntheticLegacyTimerKill(false, 0x51510001u, 0x51510001u));
    CHECK_TRUE(!shh::ShouldSwallowSyntheticLegacyTimerKill(true, 0x51510002u, 0x51510001u));
}

TEST_CASE(BuildLegacyWaitableTimerWorkerPlanLeavesOrdinarySleepsAlone)
{
    const shh::LegacyWaitableTimerWorkerPlan plan =
        shh::BuildLegacyWaitableTimerWorkerPlan(true, 1, false, true);

    CHECK_TRUE(!plan.useTrackedTimer);
    CHECK_EQ(plan.adjustedSleepMs, 1u);
}

TEST_CASE(BuildLegacyWaitableTimerWorkerPlanUsesTrackedTimerForLegacyWorkerPoll)
{
    const shh::LegacyWaitableTimerWorkerPlan plan =
        shh::BuildLegacyWaitableTimerWorkerPlan(true, 1, true, true);

    CHECK_TRUE(plan.useTrackedTimer);
    CHECK_EQ(plan.adjustedSleepMs, 1u);
}

TEST_CASE(BuildLegacyWaitableTimerWorkerPlanFallsBackToYieldWithoutTrackedTimer)
{
    const shh::LegacyWaitableTimerWorkerPlan plan =
        shh::BuildLegacyWaitableTimerWorkerPlan(true, 1, true, false);

    CHECK_TRUE(!plan.useTrackedTimer);
    CHECK_EQ(plan.adjustedSleepMs, 0u);
}

TEST_CASE(BuildLegacyThreadControlPlanLeavesOrdinaryCallsAlone)
{
    const shh::LegacyThreadControlPlan plan =
        shh::BuildLegacyThreadControlPlan(true, false, true, true);

    CHECK_TRUE(!plan.suppressOperation);
}

TEST_CASE(BuildLegacyThreadControlPlanSuppressesDeadOrSelfTargets)
{
    const shh::LegacyThreadControlPlan deadThreadPlan =
        shh::BuildLegacyThreadControlPlan(true, true, true, false);
    const shh::LegacyThreadControlPlan selfTargetPlan =
        shh::BuildLegacyThreadControlPlan(true, true, false, true);

    CHECK_TRUE(deadThreadPlan.suppressOperation);
    CHECK_TRUE(selfTargetPlan.suppressOperation);
}

TEST_CASE(BuildLegacyThreadTerminatePlanLeavesOrdinaryTerminatesAlone)
{
    const shh::LegacyThreadTerminatePlan plan =
        shh::BuildLegacyThreadTerminatePlan(true, 250, false, false, false);

    CHECK_TRUE(!plan.skipTerminateCall);
    CHECK_EQ(plan.graceWaitMs, 0u);
}

TEST_CASE(BuildLegacyThreadTerminatePlanSkipsAlreadyExitedLegacyThreads)
{
    const shh::LegacyThreadTerminatePlan plan =
        shh::BuildLegacyThreadTerminatePlan(true, 250, true, true, false);

    CHECK_TRUE(plan.skipTerminateCall);
    CHECK_EQ(plan.graceWaitMs, 0u);
}

TEST_CASE(BuildLegacyThreadTerminatePlanAddsGraceWaitForLiveLegacyThreads)
{
    const shh::LegacyThreadTerminatePlan plan =
        shh::BuildLegacyThreadTerminatePlan(true, 250, true, false, false);

    CHECK_TRUE(!plan.skipTerminateCall);
    CHECK_EQ(plan.graceWaitMs, 250u);
}
