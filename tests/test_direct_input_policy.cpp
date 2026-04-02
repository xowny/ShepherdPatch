#include "direct_input_policy.h"
#include "test_registry.h"

TEST_CASE(SanitizeMouseCooperativeFlagsLeavesOrdinaryRequestsAlone)
{
    const DWORD flags =
        shh::SanitizeMouseCooperativeFlags(false, true, DISCL_BACKGROUND | DISCL_EXCLUSIVE);

    CHECK_EQ(flags, static_cast<DWORD>(DISCL_BACKGROUND | DISCL_EXCLUSIVE));
}

TEST_CASE(SanitizeMouseCooperativeFlagsPromotesRawMouseFriendlyForegroundMode)
{
    const DWORD flags = shh::SanitizeMouseCooperativeFlags(
        true, true, DISCL_BACKGROUND | DISCL_EXCLUSIVE | DISCL_NOWINKEY);

    CHECK_EQ(flags, static_cast<DWORD>(DISCL_FOREGROUND | DISCL_NONEXCLUSIVE));
}

TEST_CASE(SanitizeMouseCooperativeFlagsLeavesNonMouseDevicesAlone)
{
    const DWORD flags =
        shh::SanitizeMouseCooperativeFlags(true, false, DISCL_BACKGROUND | DISCL_EXCLUSIVE);

    CHECK_EQ(flags, static_cast<DWORD>(DISCL_BACKGROUND | DISCL_EXCLUSIVE));
}

TEST_CASE(BuildMouseStateRecoveryPlanLeavesSuccessfulPollsAlone)
{
    const shh::DirectInputMouseStateRecoveryPlan plan =
        shh::BuildMouseStateRecoveryPlan(true, true, DI_OK, true);

    CHECK_TRUE(!plan.overrideResult);
    CHECK_TRUE(!plan.zeroState);
    CHECK_EQ(plan.result, static_cast<HRESULT>(DI_OK));
}

TEST_CASE(BuildMouseStateRecoveryPlanConvertsLostMousePollsIntoZeroState)
{
    const shh::DirectInputMouseStateRecoveryPlan inputLostPlan =
        shh::BuildMouseStateRecoveryPlan(true, true, DIERR_INPUTLOST, true);
    const shh::DirectInputMouseStateRecoveryPlan notAcquiredPlan =
        shh::BuildMouseStateRecoveryPlan(true, true, DIERR_NOTACQUIRED, true);

    CHECK_TRUE(inputLostPlan.overrideResult);
    CHECK_TRUE(inputLostPlan.zeroState);
    CHECK_EQ(inputLostPlan.result, static_cast<HRESULT>(DI_OK));
    CHECK_TRUE(notAcquiredPlan.overrideResult);
    CHECK_TRUE(notAcquiredPlan.zeroState);
    CHECK_EQ(notAcquiredPlan.result, static_cast<HRESULT>(DI_OK));
}

TEST_CASE(BuildMouseStateRecoveryPlanRequiresValidMouseStateBuffer)
{
    const shh::DirectInputMouseStateRecoveryPlan plan =
        shh::BuildMouseStateRecoveryPlan(true, true, DIERR_INPUTLOST, false);

    CHECK_TRUE(!plan.overrideResult);
    CHECK_TRUE(!plan.zeroState);
    CHECK_EQ(plan.result, static_cast<HRESULT>(DIERR_INPUTLOST));
}
