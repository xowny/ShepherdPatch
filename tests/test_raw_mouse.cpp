#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include "raw_mouse.h"
#include "test_registry.h"

TEST_CASE(ApplyRawMouseDeltaUpdatesDimouseState)
{
    DIMOUSESTATE state{};
    state.rgbButtons[0] = 0x80;

    CHECK_TRUE(shh::ApplyRawMouseDelta(&state, sizeof(state), 14, -9));
    CHECK_EQ(state.lX, 14);
    CHECK_EQ(state.lY, -9);
    CHECK_EQ(state.rgbButtons[0], 0x80);
}

TEST_CASE(ApplyRawMouseDeltaUpdatesDimouseState2)
{
    DIMOUSESTATE2 state{};
    state.lZ = 12;

    CHECK_TRUE(shh::ApplyRawMouseDelta(&state, sizeof(state), -4, 21));
    CHECK_EQ(state.lX, -4);
    CHECK_EQ(state.lY, 21);
    CHECK_EQ(state.lZ, 12);
}

TEST_CASE(ApplyRawMouseDeltaRejectsUnexpectedSizes)
{
    unsigned char state[7] = {};
    CHECK_TRUE(!shh::ApplyRawMouseDelta(&state, sizeof(state), 1, 2));
}

TEST_CASE(TransformRawMouseDeltaLeavesDefaultInputAlone)
{
    long deltaX = 12;
    long deltaY = -7;

    shh::TransformRawMouseDelta(1.0f, false, &deltaX, &deltaY);

    CHECK_EQ(deltaX, 12);
    CHECK_EQ(deltaY, -7);
}

TEST_CASE(TransformRawMouseDeltaAppliesSensitivityMultiplier)
{
    long deltaX = 8;
    long deltaY = -6;

    shh::TransformRawMouseDelta(1.5f, false, &deltaX, &deltaY);

    CHECK_EQ(deltaX, 12);
    CHECK_EQ(deltaY, -9);
}

TEST_CASE(TransformRawMouseDeltaSupportsInvertY)
{
    long deltaX = -5;
    long deltaY = 9;

    shh::TransformRawMouseDelta(1.0f, true, &deltaX, &deltaY);

    CHECK_EQ(deltaX, -5);
    CHECK_EQ(deltaY, -9);
}
