#include <d3d9.h>

#include "present_params.h"
#include "test_registry.h"

TEST_CASE(SanitizeForResetForcesWindowedRefreshSafeValues)
{
    shh::Config config;
    config.forceBorderless = true;
    config.reduceBorderlessPresentStutter = false;
    config.enableFrameRateUnlock = false;
    config.fallbackWidth = 1920;
    config.fallbackHeight = 1080;
    config.fallbackRefreshRate = 60;

    shh::PresentParams params;
    params.backBufferWidth = 0;
    params.backBufferHeight = 0;
    params.windowed = false;
    params.refreshRate = 144;
    params.swapEffect = D3DSWAPEFFECT_DISCARD;
    params.presentationInterval = 4;

    const shh::PresentParams sanitized = shh::SanitizeForReset(params, config);

    CHECK_TRUE(sanitized.windowed);
    CHECK_EQ(sanitized.backBufferWidth, 1920u);
    CHECK_EQ(sanitized.backBufferHeight, 1080u);
    CHECK_EQ(sanitized.refreshRate, 0u);
    CHECK_EQ(sanitized.swapEffect, D3DSWAPEFFECT_DISCARD);
    CHECK_EQ(sanitized.presentationInterval, 4u);
}

TEST_CASE(BuildRetryResetParamsDropsFullscreenAndUsesFallbackRate)
{
    shh::Config config;
    config.retryResetInWindowedMode = true;
    config.enableFrameRateUnlock = false;
    config.fallbackWidth = 1280;
    config.fallbackHeight = 720;
    config.fallbackRefreshRate = 60;

    shh::PresentParams params;
    params.backBufferWidth = 2560;
    params.backBufferHeight = 1440;
    params.windowed = false;
    params.refreshRate = 120;
    params.swapEffect = D3DSWAPEFFECT_DISCARD;
    params.presentationInterval = 1;

    const shh::PresentParams retryParams = shh::BuildRetryResetParams(params, config);

    CHECK_TRUE(retryParams.windowed);
    CHECK_EQ(retryParams.backBufferWidth, 1280u);
    CHECK_EQ(retryParams.backBufferHeight, 720u);
    CHECK_EQ(retryParams.refreshRate, 0u);
    CHECK_EQ(retryParams.swapEffect, D3DSWAPEFFECT_DISCARD);
    CHECK_EQ(retryParams.presentationInterval, 1u);
}

TEST_CASE(SanitizeForResetClampsMultiVsyncPresentationIntervals)
{
    shh::Config config;
    config.enableFrameRateUnlock = true;
    config.maximumPresentationInterval = 1;

    shh::PresentParams params;
    params.backBufferWidth = 1920;
    params.backBufferHeight = 1080;
    params.windowed = false;
    params.refreshRate = 60;
    params.swapEffect = D3DSWAPEFFECT_DISCARD;
    params.presentationInterval = 2;

    const shh::PresentParams sanitized = shh::SanitizeForReset(params, config);

    CHECK_EQ(sanitized.presentationInterval, 1u);
}

TEST_CASE(BuildRetryResetParamsClampsRetryPresentationIntervalWhenUnlockEnabled)
{
    shh::Config config;
    config.retryResetInWindowedMode = true;
    config.enableFrameRateUnlock = true;
    config.maximumPresentationInterval = 1;

    shh::PresentParams params;
    params.backBufferWidth = 1920;
    params.backBufferHeight = 1080;
    params.windowed = false;
    params.refreshRate = 60;
    params.swapEffect = D3DSWAPEFFECT_DISCARD;
    params.presentationInterval = 4;

    const shh::PresentParams retryParams = shh::BuildRetryResetParams(params, config);

    CHECK_TRUE(retryParams.windowed);
    CHECK_EQ(retryParams.presentationInterval, 1u);
}

TEST_CASE(SanitizeForResetForcesImmediatePresentationIntervalWhenVsyncDisabled)
{
    shh::Config config;
    config.enableFrameRateUnlock = true;
    config.maximumPresentationInterval = 1;
    config.disableVsync = true;

    shh::PresentParams params;
    params.backBufferWidth = 1920;
    params.backBufferHeight = 1080;
    params.windowed = false;
    params.refreshRate = 60;
    params.swapEffect = D3DSWAPEFFECT_DISCARD;
    params.presentationInterval = 2;

    const shh::PresentParams sanitized = shh::SanitizeForReset(params, config);

    CHECK_EQ(sanitized.presentationInterval, 0x80000000u);
}

TEST_CASE(SanitizeForResetForcesImmediatePresentationIntervalForBorderlessStutterReduction)
{
    shh::Config config;
    config.forceBorderless = true;
    config.reduceBorderlessPresentStutter = true;

    shh::PresentParams params;
    params.backBufferWidth = 1920;
    params.backBufferHeight = 1080;
    params.windowed = false;
    params.refreshRate = 60;
    params.swapEffect = D3DSWAPEFFECT_DISCARD;
    params.presentationInterval = 1;

    const shh::PresentParams sanitized = shh::SanitizeForReset(params, config);

    CHECK_TRUE(sanitized.windowed);
    CHECK_EQ(sanitized.swapEffect, D3DSWAPEFFECT_DISCARD);
    CHECK_EQ(sanitized.presentationInterval, 0x80000000u);
}

TEST_CASE(SanitizeForResetKeepsWindowedVsyncIntervalWhenBorderlessStutterReductionDisabled)
{
    shh::Config config;
    config.forceBorderless = true;
    config.reduceBorderlessPresentStutter = false;

    shh::PresentParams params;
    params.backBufferWidth = 1920;
    params.backBufferHeight = 1080;
    params.windowed = false;
    params.refreshRate = 60;
    params.swapEffect = D3DSWAPEFFECT_DISCARD;
    params.presentationInterval = 1;

    const shh::PresentParams sanitized = shh::SanitizeForReset(params, config);

    CHECK_TRUE(sanitized.windowed);
    CHECK_EQ(sanitized.swapEffect, D3DSWAPEFFECT_DISCARD);
    CHECK_EQ(sanitized.presentationInterval, 1u);
}

TEST_CASE(SanitizeForResetPromotesBorderlessExSwapEffectToFlipEx)
{
    shh::Config config;
    config.forceBorderless = true;
    config.enableFlipExSwapEffect = true;

    shh::PresentParams params;
    params.backBufferWidth = 1920;
    params.backBufferHeight = 1080;
    params.windowed = false;
    params.refreshRate = 60;
    params.swapEffect = D3DSWAPEFFECT_DISCARD;
    params.presentationInterval = 1;

    const shh::PresentParams sanitized = shh::SanitizeForReset(params, config, true);

    CHECK_TRUE(sanitized.windowed);
    CHECK_EQ(sanitized.swapEffect, D3DSWAPEFFECT_FLIPEX);
}

TEST_CASE(SanitizeForResetLeavesSwapEffectAloneWithoutFlipExSupport)
{
    shh::Config config;
    config.forceBorderless = true;
    config.enableFlipExSwapEffect = true;

    shh::PresentParams params;
    params.backBufferWidth = 1920;
    params.backBufferHeight = 1080;
    params.windowed = false;
    params.refreshRate = 60;
    params.swapEffect = D3DSWAPEFFECT_DISCARD;
    params.presentationInterval = 1;

    const shh::PresentParams sanitized = shh::SanitizeForReset(params, config, false);

    CHECK_TRUE(sanitized.windowed);
    CHECK_EQ(sanitized.swapEffect, D3DSWAPEFFECT_DISCARD);
}

TEST_CASE(SanitizeForResetClearsSuspiciousLowFullscreenRefreshWhenUnlockEnabled)
{
    shh::Config config;
    config.enableFrameRateUnlock = true;

    shh::PresentParams params;
    params.backBufferWidth = 3840;
    params.backBufferHeight = 2160;
    params.windowed = false;
    params.refreshRate = 29;
    params.swapEffect = D3DSWAPEFFECT_DISCARD;
    params.presentationInterval = 1;

    const shh::PresentParams sanitized = shh::SanitizeForReset(params, config);

    CHECK_EQ(sanitized.refreshRate, 0u);
}

TEST_CASE(SanitizeForResetUsesFallbackRefreshRateForSuspiciousFullscreenMode)
{
    shh::Config config;
    config.enableFrameRateUnlock = true;
    config.fallbackRefreshRate = 60;

    shh::PresentParams params;
    params.backBufferWidth = 3840;
    params.backBufferHeight = 2160;
    params.windowed = false;
    params.refreshRate = 29;
    params.swapEffect = D3DSWAPEFFECT_DISCARD;
    params.presentationInterval = 1;

    const shh::PresentParams sanitized = shh::SanitizeForReset(params, config);

    CHECK_EQ(sanitized.refreshRate, 60u);
}

TEST_CASE(BuildRetryResetParamsForcesImmediatePresentationIntervalWhenVsyncDisabled)
{
    shh::Config config;
    config.retryResetInWindowedMode = true;
    config.enableFrameRateUnlock = true;
    config.maximumPresentationInterval = 1;
    config.disableVsync = true;

    shh::PresentParams params;
    params.backBufferWidth = 1920;
    params.backBufferHeight = 1080;
    params.windowed = false;
    params.refreshRate = 60;
    params.swapEffect = D3DSWAPEFFECT_DISCARD;
    params.presentationInterval = 4;

    const shh::PresentParams retryParams = shh::BuildRetryResetParams(params, config);

    CHECK_TRUE(retryParams.windowed);
    CHECK_EQ(retryParams.swapEffect, D3DSWAPEFFECT_DISCARD);
    CHECK_EQ(retryParams.presentationInterval, 0x80000000u);
}

TEST_CASE(BuildRetryResetParamsForcesImmediatePresentationIntervalForBorderlessStutterReduction)
{
    shh::Config config;
    config.retryResetInWindowedMode = true;
    config.forceBorderless = true;
    config.reduceBorderlessPresentStutter = true;

    shh::PresentParams params;
    params.backBufferWidth = 1920;
    params.backBufferHeight = 1080;
    params.windowed = false;
    params.refreshRate = 60;
    params.swapEffect = D3DSWAPEFFECT_DISCARD;
    params.presentationInterval = 1;

    const shh::PresentParams retryParams =
        shh::BuildRetryResetParams(params, config);

    CHECK_TRUE(retryParams.windowed);
    CHECK_EQ(retryParams.swapEffect, D3DSWAPEFFECT_DISCARD);
    CHECK_EQ(retryParams.presentationInterval, 0x80000000u);
}

TEST_CASE(BuildRetryResetParamsPromotesBorderlessExSwapEffectToFlipEx)
{
    shh::Config config;
    config.retryResetInWindowedMode = true;
    config.forceBorderless = true;
    config.enableFlipExSwapEffect = true;

    shh::PresentParams params;
    params.backBufferWidth = 1920;
    params.backBufferHeight = 1080;
    params.windowed = false;
    params.refreshRate = 60;
    params.swapEffect = D3DSWAPEFFECT_DISCARD;
    params.presentationInterval = 1;

    const shh::PresentParams retryParams =
        shh::BuildRetryResetParams(params, config, true);

    CHECK_TRUE(retryParams.windowed);
    CHECK_EQ(retryParams.swapEffect, D3DSWAPEFFECT_FLIPEX);
}
