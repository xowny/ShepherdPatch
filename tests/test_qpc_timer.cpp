#include "qpc_timer.h"
#include "test_registry.h"

TEST_CASE(ComputeHighPrecisionTickCountAddsElapsedMilliseconds)
{
    const shh::HighPrecisionTimerState state{
        .frequency = 1000,
        .startCounter = 5000,
        .baseTickCount = 1200,
    };

    CHECK_EQ(shh::ComputeHighPrecisionTickCount(state, 5342), 1542u);
}

TEST_CASE(ComputeHighPrecisionTickCountReturnsBaseWhenCounterIsNotReady)
{
    const shh::HighPrecisionTimerState state{
        .frequency = 1000,
        .startCounter = 5000,
        .baseTickCount = 1200,
    };

    CHECK_EQ(shh::ComputeHighPrecisionTickCount(state, 5000), 1200u);

    const shh::HighPrecisionTimerState zeroFrequency{
        .frequency = 0,
        .startCounter = 5000,
        .baseTickCount = 900,
    };
    CHECK_EQ(shh::ComputeHighPrecisionTickCount(zeroFrequency, 9000), 900u);
}

TEST_CASE(ComputeHighPrecisionTickCountWrapsNaturallyLikeDWORD)
{
    const shh::HighPrecisionTimerState state{
        .frequency = 1000,
        .startCounter = 100,
        .baseTickCount = 0xfffffff0u,
    };

    CHECK_EQ(shh::ComputeHighPrecisionTickCount(state, 140), 24u);
}
