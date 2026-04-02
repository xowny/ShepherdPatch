#include "frame_rate_override.h"
#include "test_registry.h"

TEST_CASE(ResolveEngineFrameRateLeavesRequestedValueWhenUnlockDisabled)
{
    CHECK_EQ(shh::ResolveEngineFrameRate(false, 60, 30), 30u);
}

TEST_CASE(ResolveEngineFrameRateUsesConfiguredTargetWhenUnlockEnabled)
{
    CHECK_EQ(shh::ResolveEngineFrameRate(true, 60, 30), 60u);
    CHECK_EQ(shh::ResolveEngineFrameRate(true, 45, 30), 45u);
    CHECK_EQ(shh::ResolveEngineFrameRate(true, 60, 120), 60u);
}

TEST_CASE(ResolveEngineFrameRateTreatsZeroTargetAsEffectivelyUncapped)
{
    CHECK_EQ(shh::ResolveEngineFrameRate(true, 0, 30), 1000u);
}

TEST_CASE(ComputeFrameDurationMillisecondsReturnsExpectedInterval)
{
    CHECK_EQ(shh::ComputeFrameDurationMilliseconds(60), 1000.0f / 60.0f);
    CHECK_EQ(shh::ComputeFrameDurationMilliseconds(0), 0.0f);
}

TEST_CASE(ResolveLegacyPeriodicTimerDelayMsLeavesDelayAloneWhenUnlockDisabled)
{
    CHECK_EQ(shh::ResolveLegacyPeriodicTimerDelayMs(false, 60, 33), 33u);
}

TEST_CASE(ResolveLegacyPeriodicTimerDelayMsPromotesThirtyFpsTimerToTargetRate)
{
    CHECK_EQ(shh::ResolveLegacyPeriodicTimerDelayMs(true, 60, 33), 16u);
    CHECK_EQ(shh::ResolveLegacyPeriodicTimerDelayMs(true, 120, 33), 8u);
    CHECK_EQ(shh::ResolveLegacyPeriodicTimerDelayMs(true, 0, 33), 1u);
}

TEST_CASE(ResolveThirdPartyFramePacingDelayMsLeavesNonPacingSleepAlone)
{
    CHECK_EQ(shh::ResolveThirdPartyFramePacingDelayMs(false, 60, 50), 50u);
    CHECK_EQ(shh::ResolveThirdPartyFramePacingDelayMs(true, 60, 50), 50u);
}

TEST_CASE(ResolveThirdPartyFramePacingDelayMsClampsThirtyFpsCadenceToTarget)
{
    CHECK_EQ(shh::ResolveThirdPartyFramePacingDelayMs(true, 60, 33), 16u);
    CHECK_EQ(shh::ResolveThirdPartyFramePacingDelayMs(true, 120, 33), 8u);
    CHECK_EQ(shh::ResolveThirdPartyFramePacingDelayMs(true, 144, 40), 6u);
}

TEST_CASE(ResolveThirdPartyFramePacingDelayMsTreatsUnlockedAsImmediate)
{
    CHECK_EQ(shh::ResolveThirdPartyFramePacingDelayMs(true, 0, 33), 0u);
    CHECK_EQ(shh::ResolveThirdPartyFramePacingDelayMs(true, 0, 16), 0u);
}
