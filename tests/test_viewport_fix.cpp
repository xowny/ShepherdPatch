#include "config.h"
#include "test_registry.h"
#include "viewport_fix.h"

#include <d3d9.h>

TEST_CASE(ClampHudViewportSkipsFullscreenLikeRectangles)
{
    shh::Config config;
    config.enableHudViewportClamp = true;
    config.hudViewportAspectRatio = 16.0f / 9.0f;

    D3DVIEWPORT9 viewport{0, 0, 1920, 1080, 0, 1};
    CHECK_TRUE(!shh::ClampHudViewport(&viewport, 1920.0f, 1080.0f, config));
    CHECK_EQ(viewport.Width, 1920u);
}

TEST_CASE(ClampHudViewportTrimsExtraWideHudRect)
{
    shh::Config config;
    config.enableHudViewportClamp = true;
    config.hudViewportAspectRatio = 16.0f / 9.0f;

    D3DVIEWPORT9 viewport{0, 0, 2200, 900, 0, 1};
    CHECK_TRUE(shh::ClampHudViewport(&viewport, 2560.0f, 1080.0f, config));
    CHECK_EQ(viewport.Height, 900u);
    CHECK_TRUE(viewport.Width < 2200u);
    CHECK_EQ(viewport.X, (2200u - viewport.Width) / 2u);
}

TEST_CASE(ClampHudViewportRequiresWideHud)
{
    shh::Config config;
    config.enableHudViewportClamp = true;
    config.hudViewportAspectRatio = 1.6f;

    D3DVIEWPORT9 viewport{0, 0, 1600, 1000, 0, 1};
    CHECK_TRUE(!shh::ClampHudViewport(&viewport, 1920.0f, 1080.0f, config));
}
