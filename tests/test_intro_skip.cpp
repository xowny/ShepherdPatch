#include "intro_skip.h"
#include "test_registry.h"

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
