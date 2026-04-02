#pragma once

#include <cstdint>

namespace shh
{
struct LostDeviceRecoveryState
{
    bool deviceLossObserved = false;
    bool userInitiatedClose = false;
    std::uint32_t recoveryDeadlineTick = 0;
};

std::uint32_t ArmLostDeviceRecoveryDeadline(std::uint32_t nowTick, std::uint32_t durationMs);
bool IsLostDeviceRecoveryActive(const LostDeviceRecoveryState& state, std::uint32_t nowTick);
void NoteLostDeviceRecoveryWindow(LostDeviceRecoveryState* state, std::uint32_t nowTick,
                                  std::uint32_t durationMs, bool markDeviceLoss);
void NoteLostDeviceSignal(LostDeviceRecoveryState* state, std::uint32_t nowTick,
                          std::uint32_t durationMs);
void NoteUserInitiatedClose(LostDeviceRecoveryState* state);
void ClearLostDeviceRecovery(LostDeviceRecoveryState* state);
bool ShouldSuppressLostDeviceShutdown(const LostDeviceRecoveryState& state,
                                      std::uint32_t nowTick);
} // namespace shh
