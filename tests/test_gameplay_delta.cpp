#include "gameplay_delta.h"
#include "test_registry.h"

TEST_CASE(BuildGameplayDeltaPlanLeavesOriginalDeltaWhenUnlockDisabled)
{
    const shh::GameplayDeltaPlan plan =
        shh::BuildGameplayDeltaPlan(false, 60, 1.0f / 60.0f, 1.0f / 30.0f);

    CHECK_EQ(plan.effectiveDeltaSeconds, 1.0f / 30.0f);
}

TEST_CASE(BuildGameplayDeltaPlanUsesMeasuredDeltaWhenUnlocked)
{
    const shh::GameplayDeltaPlan plan =
        shh::BuildGameplayDeltaPlan(true, 0, 1.0f / 144.0f, 1.0f / 30.0f);

    CHECK_TRUE(plan.effectiveDeltaSeconds > 0.0069f && plan.effectiveDeltaSeconds < 0.0070f);
}

TEST_CASE(BuildGameplayDeltaPlanUsesTargetCadenceWhenFramesArriveTooFast)
{
    const shh::GameplayDeltaPlan plan =
        shh::BuildGameplayDeltaPlan(true, 60, 1.0f / 120.0f, 1.0f / 30.0f);

    CHECK_TRUE(plan.effectiveDeltaSeconds > 0.0166f && plan.effectiveDeltaSeconds < 0.0167f);
}

TEST_CASE(BuildGameplayDeltaPlanPreservesRealTimeWhenFramesArriveTooSlow)
{
    const shh::GameplayDeltaPlan plan =
        shh::BuildGameplayDeltaPlan(true, 60, 0.022f, 1.0f / 30.0f);

    CHECK_TRUE(plan.effectiveDeltaSeconds > 0.0219f && plan.effectiveDeltaSeconds < 0.0221f);
}

TEST_CASE(BuildGameplayDeltaPlanFallsBackToTargetCadenceWhenMeasurementIsMissing)
{
    const shh::GameplayDeltaPlan plan =
        shh::BuildGameplayDeltaPlan(true, 60, 0.0f, 1.0f / 30.0f);

    CHECK_TRUE(plan.effectiveDeltaSeconds > 0.0166f && plan.effectiveDeltaSeconds < 0.0167f);
}
