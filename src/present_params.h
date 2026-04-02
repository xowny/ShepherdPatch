#pragma once

#include "config.h"

#include <cstdint>

namespace shh
{
struct PresentParams
{
    std::uint32_t backBufferWidth = 0;
    std::uint32_t backBufferHeight = 0;
    bool windowed = false;
    std::uint32_t refreshRate = 0;
    std::uint32_t swapEffect = 0;
    std::uint32_t presentationInterval = 0;
};

PresentParams SanitizeForReset(const PresentParams& params, const Config& config,
                               bool supportsFlipEx = false);
PresentParams BuildRetryResetParams(const PresentParams& params, const Config& config,
                                    bool supportsFlipEx = false);
} // namespace shh
