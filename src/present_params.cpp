#include "present_params.h"

#include <d3d9.h>

namespace shh
{
namespace
{
constexpr std::uint32_t kPresentationIntervalImmediate = 0x80000000u;
constexpr std::uint32_t kMinimumModernFullscreenRefreshRate = 50u;

bool ShouldForceImmediatePresentation(const PresentParams& params, const Config& config)
{
    if (config.disableVsync)
    {
        return true;
    }

    return config.forceBorderless && params.windowed &&
           config.reduceBorderlessPresentStutter;
}

bool ShouldPromoteFlipEx(const PresentParams& params, const Config& config,
                         bool supportsFlipEx)
{
    return supportsFlipEx && config.enableFlipExSwapEffect && config.forceBorderless &&
           params.windowed && params.swapEffect == D3DSWAPEFFECT_DISCARD;
}

std::uint32_t ResolveDimension(std::uint32_t currentValue, std::uint32_t fallbackValue)
{
    return currentValue != 0 ? currentValue : fallbackValue;
}

void ApplyWindowedMode(PresentParams& params)
{
    params.windowed = true;
    params.refreshRate = 0;
}

void SanitizeRefreshRate(PresentParams& params, const Config& config)
{
    if (params.windowed)
    {
        params.refreshRate = 0;
        return;
    }

    if (params.refreshRate == 0)
    {
        return;
    }

    const bool wantsModernPresentation =
        config.enableFrameRateUnlock || config.disableVsync;
    const bool suspiciousLowFullscreenRefresh =
        wantsModernPresentation && params.refreshRate < kMinimumModernFullscreenRefreshRate;
    if (!suspiciousLowFullscreenRefresh)
    {
        return;
    }

    params.refreshRate =
        config.fallbackRefreshRate != 0 ? config.fallbackRefreshRate : 0;
}

void ClampPresentationInterval(PresentParams& params, const Config& config)
{
    if (ShouldForceImmediatePresentation(params, config))
    {
        params.presentationInterval = kPresentationIntervalImmediate;
        return;
    }

    if (!config.enableFrameRateUnlock || config.maximumPresentationInterval == 0)
    {
        return;
    }

    if (params.presentationInterval == 0 ||
        params.presentationInterval == kPresentationIntervalImmediate)
    {
        return;
    }

    if (params.presentationInterval > config.maximumPresentationInterval)
    {
        params.presentationInterval = config.maximumPresentationInterval;
    }
}

void PromoteSwapEffect(PresentParams& params, const Config& config, bool supportsFlipEx)
{
    if (!ShouldPromoteFlipEx(params, config, supportsFlipEx))
    {
        return;
    }

    params.swapEffect = D3DSWAPEFFECT_FLIPEX;
}
} // namespace

PresentParams SanitizeForReset(const PresentParams& params, const Config& config,
                               bool supportsFlipEx)
{
    PresentParams sanitized = params;
    sanitized.backBufferWidth = ResolveDimension(sanitized.backBufferWidth, config.fallbackWidth);
    sanitized.backBufferHeight =
        ResolveDimension(sanitized.backBufferHeight, config.fallbackHeight);

    if (config.forceBorderless)
    {
        ApplyWindowedMode(sanitized);
    }

    SanitizeRefreshRate(sanitized, config);
    PromoteSwapEffect(sanitized, config, supportsFlipEx);
    ClampPresentationInterval(sanitized, config);
    return sanitized;
}

PresentParams BuildRetryResetParams(const PresentParams& params, const Config& config,
                                    bool supportsFlipEx)
{
    if (!config.retryResetInWindowedMode)
    {
        return SanitizeForReset(params, config, supportsFlipEx);
    }

    PresentParams retryParams = params;
    if (config.fallbackWidth != 0)
    {
        retryParams.backBufferWidth = config.fallbackWidth;
    }

    if (config.fallbackHeight != 0)
    {
        retryParams.backBufferHeight = config.fallbackHeight;
    }

    ApplyWindowedMode(retryParams);
    PromoteSwapEffect(retryParams, config, supportsFlipEx);
    ClampPresentationInterval(retryParams, config);
    return retryParams;
}
} // namespace shh
