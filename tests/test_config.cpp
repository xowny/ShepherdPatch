#include "config.h"
#include "test_registry.h"

#include <string_view>

TEST_CASE(ParseConfigReadsBoolAndIntegerOverrides)
{
    constexpr std::string_view configText =
        "# comment\n"
        "EnableSafeResolutionChanges = false\n"
        "EnableDpiAwareness = false\n"
        "EnableUltrawideFovFix = false\n"
        "ForceBorderless = false\n"
        "RetryResetInWindowedMode = true\n"
        "EnableHighPrecisionTiming = false\n"
        "RequestHighResolutionTimer = false\n"
        "EnableRawMouseInput = false\n"
        "EnablePreciseSleepShim = false\n"
        "EnableCrashDumps = false\n"
        "EnableFrameRateUnlock = false\n"
        "SynchronizeEngineVars = false\n"
        "EnableGamesMmcssProfile = false\n"
        "DisablePowerThrottling = false\n"
        "DisableVsync = true\n"
        "DisableLegacyPeriodicIconicTimer = false\n"
        "ReduceLegacyWaitableTimerPolling = false\n"
        "HardenLegacyGraphicsRecovery = false\n"
        "HardenLegacyThreadWrapper = false\n"
        "HardenDirectInputMouseDevice = false\n"
        "ReduceBorderlessPresentStutter = false\n"
        "ReduceMenuMovieStutter = false\n"
        "EnableFlipExSwapEffect = false\n"
        "LegacyGraphicsRetryDelayMilliseconds = 250\n"
        "LegacyThreadTerminateGraceMilliseconds = 400\n"
        "RawMouseSensitivity = 1.5\n"
        "InvertRawMouseY = true\n"
        "PresentWakeLeadMilliseconds = 0.75\n"
        "TargetFrameRate = 120\n"
        "MaximumPresentationInterval = 2\n"
        "FallbackWidth = 1600\n"
        "FallbackHeight = 900\n"
        "FallbackRefreshRate = 60\n"
        "EnableHudViewportClamp = false\n"
        "HudViewportAspectRatio = 1.6\n";

    const shh::Config config = shh::ParseConfig(configText);

    CHECK_TRUE(!config.enableSafeResolutionChanges);
    CHECK_TRUE(!config.enableDpiAwareness);
    CHECK_TRUE(!config.enableUltrawideFovFix);
    CHECK_TRUE(!config.forceBorderless);
    CHECK_TRUE(config.retryResetInWindowedMode);
    CHECK_TRUE(!config.enableHighPrecisionTiming);
    CHECK_TRUE(!config.requestHighResolutionTimer);
    CHECK_TRUE(!config.enableRawMouseInput);
    CHECK_TRUE(!config.enablePreciseSleepShim);
    CHECK_TRUE(!config.enableCrashDumps);
    CHECK_TRUE(!config.enableFrameRateUnlock);
    CHECK_TRUE(!config.synchronizeEngineVars);
    CHECK_TRUE(!config.enableGamesMmcssProfile);
    CHECK_TRUE(!config.disablePowerThrottling);
    CHECK_TRUE(config.disableVsync);
    CHECK_TRUE(!config.disableLegacyPeriodicIconicTimer);
    CHECK_TRUE(!config.reduceLegacyWaitableTimerPolling);
    CHECK_TRUE(!config.hardenLegacyGraphicsRecovery);
    CHECK_TRUE(!config.hardenLegacyThreadWrapper);
    CHECK_TRUE(!config.hardenDirectInputMouseDevice);
    CHECK_TRUE(!config.reduceBorderlessPresentStutter);
    CHECK_TRUE(!config.reduceMenuMovieStutter);
    CHECK_TRUE(!config.enableFlipExSwapEffect);
    CHECK_EQ(config.legacyGraphicsRetryDelayMilliseconds, 250u);
    CHECK_EQ(config.legacyThreadTerminateGraceMilliseconds, 400u);
    CHECK_EQ(config.rawMouseSensitivity, 1.5f);
    CHECK_TRUE(config.invertRawMouseY);
    CHECK_EQ(config.presentWakeLeadMilliseconds, 0.75f);
    CHECK_EQ(config.targetFrameRate, 120u);
    CHECK_EQ(config.maximumPresentationInterval, 2u);
    CHECK_EQ(config.fallbackWidth, 1600u);
    CHECK_EQ(config.fallbackHeight, 900u);
    CHECK_EQ(config.fallbackRefreshRate, 60u);
    CHECK_TRUE(!config.enableHudViewportClamp);
    CHECK_EQ(config.hudViewportAspectRatio, 1.6f);
}

TEST_CASE(TrimRemovesLeadingAndTrailingWhitespace)
{
    const std::string trimmed = shh::Trim(" \t  borderless = true \r\n");
    CHECK_EQ(trimmed, "borderless = true");
}

TEST_CASE(ParseConfigUsesConservativeDefaultsWhenFileIsMissing)
{
    const shh::Config config = shh::ParseConfig({});

    CHECK_TRUE(config.enableSafeResolutionChanges);
    CHECK_TRUE(config.enableDpiAwareness);
    CHECK_TRUE(config.enableUltrawideFovFix);
    CHECK_TRUE(!config.forceBorderless);
    CHECK_TRUE(config.retryResetInWindowedMode);
    CHECK_TRUE(config.enableHighPrecisionTiming);
    CHECK_TRUE(config.requestHighResolutionTimer);
    CHECK_TRUE(config.enableRawMouseInput);
    CHECK_TRUE(config.enablePreciseSleepShim);
    CHECK_TRUE(config.enableCrashDumps);
    CHECK_TRUE(config.enableFrameRateUnlock);
    CHECK_TRUE(config.synchronizeEngineVars);
    CHECK_TRUE(config.enableGamesMmcssProfile);
    CHECK_TRUE(config.disablePowerThrottling);
    CHECK_TRUE(!config.disableVsync);
    CHECK_TRUE(config.disableLegacyPeriodicIconicTimer);
    CHECK_TRUE(config.reduceLegacyWaitableTimerPolling);
    CHECK_TRUE(config.hardenLegacyGraphicsRecovery);
    CHECK_TRUE(config.hardenLegacyThreadWrapper);
    CHECK_TRUE(config.hardenDirectInputMouseDevice);
    CHECK_TRUE(config.reduceBorderlessPresentStutter);
    CHECK_TRUE(config.reduceMenuMovieStutter);
    CHECK_TRUE(config.enableFlipExSwapEffect);
    CHECK_EQ(config.legacyGraphicsRetryDelayMilliseconds, 100u);
    CHECK_EQ(config.legacyThreadTerminateGraceMilliseconds, 250u);
    CHECK_EQ(config.rawMouseSensitivity, 1.0f);
    CHECK_TRUE(!config.invertRawMouseY);
    CHECK_EQ(config.presentWakeLeadMilliseconds, 0.0f);
    CHECK_EQ(config.targetFrameRate, 60u);
    CHECK_EQ(config.maximumPresentationInterval, 1u);
    CHECK_TRUE(config.enableHudViewportClamp);
    CHECK_EQ(config.hudViewportAspectRatio, 16.0f / 9.0f);
}
