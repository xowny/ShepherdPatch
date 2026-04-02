#include "lost_device_policy.h"
#include "test_registry.h"

TEST_CASE(LostDeviceRecoveryActivatesForFutureDeadline)
{
    shh::LostDeviceRecoveryState state = {};
    state.recoveryDeadlineTick = shh::ArmLostDeviceRecoveryDeadline(100u, 500u);

    CHECK_TRUE(shh::IsLostDeviceRecoveryActive(state, 200u));
    CHECK_TRUE(!shh::IsLostDeviceRecoveryActive(state, 700u));
}

TEST_CASE(LostDeviceShutdownSuppressionRequiresObservedLoss)
{
    shh::LostDeviceRecoveryState state = {};
    shh::NoteLostDeviceSignal(&state, 100u, 500u);

    CHECK_TRUE(shh::ShouldSuppressLostDeviceShutdown(state, 200u));

    shh::NoteUserInitiatedClose(&state);
    CHECK_TRUE(!shh::ShouldSuppressLostDeviceShutdown(state, 200u));
}

TEST_CASE(ClearLostDeviceRecoveryResetsSuppressionState)
{
    shh::LostDeviceRecoveryState state = {};
    shh::NoteLostDeviceSignal(&state, 100u, 500u);

    shh::ClearLostDeviceRecovery(&state);

    CHECK_TRUE(!state.deviceLossObserved);
    CHECK_TRUE(!state.userInitiatedClose);
    CHECK_EQ(state.recoveryDeadlineTick, 0u);
    CHECK_TRUE(!shh::ShouldSuppressLostDeviceShutdown(state, 200u));
}

TEST_CASE(NoteLostDeviceRecoveryWindowDoesNotSuppressWithoutObservedLoss)
{
    shh::LostDeviceRecoveryState state = {};

    shh::NoteLostDeviceRecoveryWindow(&state, 100u, 800u, false);

    CHECK_TRUE(shh::IsLostDeviceRecoveryActive(state, 200u));
    CHECK_TRUE(!state.deviceLossObserved);
    CHECK_TRUE(!shh::ShouldSuppressLostDeviceShutdown(state, 200u));
}

TEST_CASE(NoteLostDeviceRecoveryWindowDoesNotShortenActiveDeadline)
{
    shh::LostDeviceRecoveryState state = {};

    shh::NoteLostDeviceRecoveryWindow(&state, 100u, 800u, false);
    const std::uint32_t initialDeadline = state.recoveryDeadlineTick;

    shh::NoteLostDeviceRecoveryWindow(&state, 200u, 100u, false);

    CHECK_EQ(state.recoveryDeadlineTick, initialDeadline);
    CHECK_TRUE(!state.deviceLossObserved);
}
