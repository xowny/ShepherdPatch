#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace shh
{
struct EngineVarsOverrides
{
    bool syncFullscreenToBorderless = false;
    std::optional<std::uint32_t> screenRefresh;
};

std::string ApplyEngineVarsOverrides(std::string_view text,
                                     const EngineVarsOverrides& overrides);
} // namespace shh
