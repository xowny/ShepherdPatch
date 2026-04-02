#include "crash_dump.h"
#include "test_registry.h"

#include <filesystem>

TEST_CASE(BuildCrashDumpPathUsesTimestampAndProcessId)
{
    const shh::CrashDumpTimestamp timestamp{
        .year = 2026,
        .month = 3,
        .day = 30,
        .hour = 11,
        .minute = 7,
        .second = 5,
        .milliseconds = 42,
    };

    const std::filesystem::path dumpPath = shh::BuildCrashDumpPath(
        "C:\\Games\\Silent Hill Homecoming\\Bin", "Silent Hill: Homecoming", timestamp, 4242);

    CHECK_EQ(dumpPath,
             std::filesystem::path(
                 "C:\\Games\\Silent Hill Homecoming\\Bin\\Silent_Hill_Homecoming-crash-"
                 "20260330-110705-042-pid4242.dmp"));
}

TEST_CASE(BuildCrashDumpPathFallsBackToDefaultPrefixWhenNameIsEmpty)
{
    const shh::CrashDumpTimestamp timestamp{
        .year = 2026,
        .month = 3,
        .day = 30,
        .hour = 9,
        .minute = 0,
        .second = 1,
        .milliseconds = 7,
    };

    const std::filesystem::path dumpPath =
        shh::BuildCrashDumpPath("C:\\DumpRoot", "", timestamp, 77);

    CHECK_EQ(dumpPath,
             std::filesystem::path(
                 "C:\\DumpRoot\\ShepherdPatch-crash-20260330-090001-007-pid77.dmp"));
}
