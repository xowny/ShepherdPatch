#include "engine_vars_overrides.h"
#include "test_registry.h"

#include <string_view>

TEST_CASE(ApplyEngineVarsOverridesUpdatesExistingModernDefaults)
{
    constexpr std::string_view input =
        "fps=true\r\n"
        "FullScreen=true\r\n"
        "ScreenRefresh=30\r\n";

    shh::EngineVarsOverrides overrides = {};
    overrides.syncFullscreenToBorderless = true;
    overrides.screenRefresh = 60;

    const std::string output = shh::ApplyEngineVarsOverrides(input, overrides);

    CHECK_TRUE(output.find("FullScreen=false") != std::string::npos);
    CHECK_TRUE(output.find("ScreenRefresh=60") != std::string::npos);
}

TEST_CASE(ApplyEngineVarsOverridesAppendsMissingKeys)
{
    constexpr std::string_view input =
        "fps=true\n"
        "mousespeed=4.000000\n";

    shh::EngineVarsOverrides overrides = {};
    overrides.syncFullscreenToBorderless = true;

    const std::string output = shh::ApplyEngineVarsOverrides(input, overrides);

    CHECK_TRUE(output.find("FullScreen=false") != std::string::npos);
}

TEST_CASE(ApplyEngineVarsOverridesLeavesUnrelatedAccessibilityAndHintKeysUntouched)
{
    constexpr std::string_view input =
        "subtitles=true\n"
        "tips=false\n"
        "rumble=false\n"
        "FullScreen=true\n";

    shh::EngineVarsOverrides overrides = {};
    overrides.syncFullscreenToBorderless = true;

    const std::string output = shh::ApplyEngineVarsOverrides(input, overrides);

    CHECK_TRUE(output.find("subtitles=true") != std::string::npos);
    CHECK_TRUE(output.find("tips=false") != std::string::npos);
    CHECK_TRUE(output.find("rumble=false") != std::string::npos);
    CHECK_TRUE(output.find("FullScreen=false") != std::string::npos);
}
