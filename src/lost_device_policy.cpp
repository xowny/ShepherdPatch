#include "lost_device_policy.h"

#include <cstdint>

namespace shh
{
namespace
{
bool IsFutureTick(std::uint32_t nowTick, std::uint32_t deadlineTick)
{
    return static_cast<std::int32_t>(deadlineTick - nowTick) > 0;
}
} // namespace

std::uint32_t ArmLostDeviceRecoveryDeadline(std::uint32_t nowTick, std::uint32_t durationMs)
{
    return nowTick + durationMs;
}

bool IsLostDeviceRecoveryActive(const LostDeviceRecoveryState& state, std::uint32_t nowTick)
{
    return state.recoveryDeadlineTick != 0 && IsFutureTick(nowTick, state.recoveryDeadlineTick);
}

void NoteLostDeviceRecoveryWindow(LostDeviceRecoveryState* state, std::uint32_t nowTick,
                                  std::uint32_t durationMs, bool markDeviceLoss)
{
    if (state == nullptr)
    {
        return;
    }

    if (markDeviceLoss)
    {
        NoteLostDeviceSignal(state, nowTick, durationMs);
        return;
    }

    const std::uint32_t newDeadline = ArmLostDeviceRecoveryDeadline(nowTick, durationMs);
    if (!IsLostDeviceRecoveryActive(*state, nowTick) ||
        static_cast<std::int32_t>(newDeadline - state->recoveryDeadlineTick) > 0)
    {
        state->recoveryDeadlineTick = newDeadline;
    }
}

void NoteLostDeviceSignal(LostDeviceRecoveryState* state, std::uint32_t nowTick,
                          std::uint32_t durationMs)
{
    if (state == nullptr)
    {
        return;
    }

    state->deviceLossObserved = true;
    state->recoveryDeadlineTick = ArmLostDeviceRecoveryDeadline(nowTick, durationMs);
}

void NoteUserInitiatedClose(LostDeviceRecoveryState* state)
{
    if (state == nullptr)
    {
        return;
    }

    state->userInitiatedClose = true;
}

void ClearLostDeviceRecovery(LostDeviceRecoveryState* state)
{
    if (state == nullptr)
    {
        return;
    }

    state->deviceLossObserved = false;
    state->userInitiatedClose = false;
    state->recoveryDeadlineTick = 0;
}

bool ShouldSuppressLostDeviceShutdown(const LostDeviceRecoveryState& state,
                                      std::uint32_t nowTick)
{
    return state.deviceLossObserved && !state.userInitiatedClose &&
           IsLostDeviceRecoveryActive(state, nowTick);
}
} // namespace shh
