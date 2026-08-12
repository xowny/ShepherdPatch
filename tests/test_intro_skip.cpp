#include "intro_skip.h"
#include "test_registry.h"

#include <array>

TEST_CASE(MenuMovieWaitPlanTargetsKnownMenuMoviesWithoutSkippingThem)
{
    const shh::MenuMovieWaitPlan plan =
        shh::BuildMenuMovieWaitPlan(true, "Engine\\movies\\Start_Loop.bik");

    CHECK_TRUE(plan.trackHandle);
    CHECK_TRUE(plan.forceZeroWait);
}

TEST_CASE(MenuMovieWaitPlanSupportsAttractMovieToo)
{
    const shh::MenuMovieWaitPlan plan =
        shh::BuildMenuMovieWaitPlan(true, "Engine\\movies\\SHH_ATTRACT.bik");

    CHECK_TRUE(plan.trackHandle);
    CHECK_TRUE(plan.forceZeroWait);
}

TEST_CASE(MenuMovieWaitPlanLeavesGameplayCutscenesUntouched)
{
    const shh::MenuMovieWaitPlan plan =
        shh::BuildMenuMovieWaitPlan(true, "Engine\\movies\\CIN_M03_030.bik");

    CHECK_TRUE(!plan.trackHandle);
    CHECK_TRUE(!plan.forceZeroWait);
}

TEST_CASE(MenuMovieWaitPlanDisablesCleanlyWhenToggleIsOff)
{
    const shh::MenuMovieWaitPlan plan =
        shh::BuildMenuMovieWaitPlan(false, "Engine\\movies\\Start_Loop.bik");

    CHECK_TRUE(!plan.trackHandle);
    CHECK_TRUE(!plan.forceZeroWait);
}

TEST_CASE(StartupLogoPatchRecognizesOnlyTheSupportedGlobalArchive)
{
    CHECK_TRUE(shh::IsSupportedGlobalPakPath(
        "Engine\\pak\\pc\\GLOBAL.PAK", shh::kSupportedGlobalPakSize));
    CHECK_TRUE(!shh::IsSupportedGlobalPakPath(
        "Engine\\pak\\pc\\LOCAL.PAK", shh::kSupportedGlobalPakSize));
    CHECK_TRUE(!shh::IsSupportedGlobalPakPath(
        "GLOBAL.PAK", shh::kSupportedGlobalPakSize - 1));
}

TEST_CASE(StartupLogoPatchChangesValidatedTimerBytesOnly)
{
    std::array<std::uint8_t, 8> bytes{'x', 'x', '1', 'x', 'x', 'x', 'x', 'x'};
    const auto result = shh::PatchStartupLogoTimers(0x579E30D, bytes);

    CHECK_EQ(result.patchedBytes, 1u);
    CHECK_EQ(result.mismatchedBytes, 0u);
    CHECK_EQ(bytes[2], static_cast<std::uint8_t>('0'));
    CHECK_EQ(bytes[1], static_cast<std::uint8_t>('x'));
}

TEST_CASE(StartupLogoPatchRejectsUnexpectedArchiveData)
{
    std::array<std::uint8_t, 1> bytes{'9'};
    const auto result = shh::PatchStartupLogoTimers(0x579EAF2, bytes);

    CHECK_EQ(result.patchedBytes, 0u);
    CHECK_EQ(result.mismatchedBytes, 1u);
    CHECK_EQ(bytes[0], static_cast<std::uint8_t>('9'));
}
