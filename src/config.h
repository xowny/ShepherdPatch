#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace shh
{
struct Config
{
    bool enableSafeResolutionChanges = true;
    bool enableDpiAwareness = true;
    bool enableUltrawideFovFix = true;
    bool forceBorderless = false;
    bool retryResetInWindowedMode = true;
    bool enableHighPrecisionTiming = true;
    bool requestHighResolutionTimer = true;
    bool enableRawMouseInput = true;
    bool enablePreciseSleepShim = true;
    bool enableCrashDumps = true;
    bool enableFrameRateUnlock = true;
    bool synchronizeEngineVars = true;
    float rawMouseSensitivity = 1.0f;
    bool invertRawMouseY = false;
    bool enableGamesMmcssProfile = true;
    bool disablePowerThrottling = true;
    bool disableVsync = false;
    bool disableLegacyPeriodicIconicTimer = true;
    bool reduceLegacyWaitableTimerPolling = true;
    bool hardenLegacyGraphicsRecovery = true;
    bool hardenLegacyThreadWrapper = true;
    bool hardenDirectInputMouseDevice = true;
    bool reduceBorderlessPresentStutter = true;
    bool reduceMenuMovieStutter = true;
    bool enableFlipExSwapEffect = true;
    std::uint32_t legacyGraphicsRetryDelayMilliseconds = 100;
    std::uint32_t legacyThreadTerminateGraceMilliseconds = 250;
    float presentWakeLeadMilliseconds = 0.0f;
    std::uint32_t targetFrameRate = 60;
    std::uint32_t fallbackWidth = 0;
    std::uint32_t fallbackHeight = 0;
    std::uint32_t fallbackRefreshRate = 0;
    std::uint32_t maximumPresentationInterval = 1;
    bool enableHudViewportClamp = true;
    float hudViewportAspectRatio = 16.0f / 9.0f;
};

Config ParseConfig(std::string_view text);
std::string Trim(std::string_view value);
} // namespace shh
