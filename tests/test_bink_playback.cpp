#include "bink_playback.h"
#include "test_registry.h"

TEST_CASE(ShouldTraceBinkPlaybackMatchesKnownMenuMovies)
{
    CHECK_TRUE(shh::ShouldTraceBinkPlayback("Engine\\movies\\Start_Loop.bik"));
    CHECK_TRUE(shh::ShouldTraceBinkPlayback("C:/Games/SHH/Engine/movies/SHH_ATTRACT.bik"));
}

TEST_CASE(ShouldTraceBinkPlaybackIgnoresGameplayMovies)
{
    CHECK_TRUE(!shh::ShouldTraceBinkPlayback("Engine\\movies\\CIN_M03_030.bik"));
    CHECK_TRUE(!shh::ShouldTraceBinkPlayback("Engine\\movies\\hotel_intro.bik"));
}

TEST_CASE(GetBinkTrackedFunctionNameUsesStableLabels)
{
    CHECK_EQ(shh::GetBinkTrackedFunctionName(shh::BinkTrackedFunction::Wait), "BinkWait");
    CHECK_EQ(shh::GetBinkTrackedFunctionName(shh::BinkTrackedFunction::DoFrame),
             "BinkDoFrame");
    CHECK_EQ(shh::GetBinkTrackedFunctionName(shh::BinkTrackedFunction::NextFrame),
             "BinkNextFrame");
    CHECK_EQ(shh::GetBinkTrackedFunctionName(shh::BinkTrackedFunction::Pause), "BinkPause");
}
