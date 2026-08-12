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
    constexpr std::uint64_t start = 0x579E30F;
    std::array<std::uint8_t, 0xAC6> bytes{};
    constexpr std::array<std::pair<std::size_t, std::uint8_t>, 16> signature{{
        {0x000, '1'}, {0x0F5, '1'}, {0x164, '2'}, {0x21A, '1'},
        {0x2CD, '1'}, {0x3EF, '1'}, {0x468, '2'}, {0x539, '1'},
        {0x607, '1'}, {0x6BF, '1'}, {0x757, '1'}, {0x7E3, '4'},
        {0x898, '1'}, {0x915, '1'}, {0x9E1, '2'}, {0xAC5, '1'},
    }};
    for (const auto [offset, value] : signature)
        bytes[offset] = value;
    const auto result = shh::PatchStartupLogoTimers(start, bytes);

    CHECK_EQ(result.patchedBytes, 16u);
    CHECK_EQ(result.mismatchedBytes, 0u);
    for (const auto [offset, value] : signature)
        CHECK_EQ(bytes[offset], static_cast<std::uint8_t>('0'));
}

TEST_CASE(StartupLogoPatchRejectsUnexpectedArchiveData)
{
    std::array<std::uint8_t, 0xAC6> bytes{};
    const auto result = shh::PatchStartupLogoTimers(0x579E30F, bytes);

    CHECK_EQ(result.patchedBytes, 0u);
    CHECK_EQ(result.mismatchedBytes, 1u);
    CHECK_EQ(bytes[0], static_cast<std::uint8_t>(0));
}

TEST_CASE(StartupLogoPatchDoesNotModifyPartialSignatureReads)
{
    std::array<std::uint8_t, 1> bytes{'1'};
    const auto result = shh::PatchStartupLogoTimers(0x579E30F, bytes);

    CHECK_EQ(result.patchedBytes, 0u);
    CHECK_EQ(result.mismatchedBytes, 0u);
    CHECK_EQ(bytes[0], static_cast<std::uint8_t>('1'));
}
