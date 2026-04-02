#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

namespace shh
{
struct CrashDumpTimestamp
{
    std::uint16_t year = 0;
    std::uint16_t month = 0;
    std::uint16_t day = 0;
    std::uint16_t hour = 0;
    std::uint16_t minute = 0;
    std::uint16_t second = 0;
    std::uint16_t milliseconds = 0;
};

std::filesystem::path BuildCrashDumpPath(const std::filesystem::path& directory,
                                         std::string_view processName,
                                         const CrashDumpTimestamp& timestamp,
                                         std::uint32_t processId);
} // namespace shh
