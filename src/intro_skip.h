#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace shh
{
struct MenuMovieWaitPlan
{
    bool trackHandle = false;
    bool forceZeroWait = false;
};
bool ShouldApplyMenuMovieWaitFix(std::string_view moviePath);
MenuMovieWaitPlan BuildMenuMovieWaitPlan(bool reduceMenuMovieStutter,
                                         std::string_view moviePath);

constexpr std::uint64_t kSupportedGlobalPakSize = 159416320;

struct StartupLogoPatchResult
{
    std::size_t patchedBytes = 0;
    std::size_t mismatchedBytes = 0;
};

bool IsSupportedGlobalPakPath(std::string_view path, std::uint64_t fileSize);
StartupLogoPatchResult PatchStartupLogoTimers(std::uint64_t fileOffset,
                                              std::span<std::uint8_t> bytes);
} // namespace shh
