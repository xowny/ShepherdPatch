#include "render_hook_targets.h"
#include "test_registry.h"

#include <cstdint>

TEST_CASE(ResolveRenderCallTargetPrefersCodeGateway)
{
    const shh::RenderHookTargets targets{
        .vtableOriginal = static_cast<std::uintptr_t>(0x10001000u),
        .codeGateway = static_cast<std::uintptr_t>(0x20002000u),
    };

    CHECK_EQ(shh::ResolveRenderCallTarget(targets),
             static_cast<std::uintptr_t>(0x20002000u));
}

TEST_CASE(ResolveRenderCallTargetFallsBackToVtableOriginal)
{
    const shh::RenderHookTargets targets{
        .vtableOriginal = static_cast<std::uintptr_t>(0x10001000u),
        .codeGateway = 0,
    };

    CHECK_EQ(shh::ResolveRenderCallTarget(targets),
             static_cast<std::uintptr_t>(0x10001000u));
}

TEST_CASE(ShouldInstallRenderCodeDetourAcceptsCleanOriginalTarget)
{
    const shh::RenderHookTargets targets{
        .vtableOriginal = static_cast<std::uintptr_t>(0x10001000u),
        .codeGateway = 0,
    };

    CHECK_TRUE(shh::ShouldInstallRenderCodeDetour(
        targets, static_cast<std::uintptr_t>(0x30003000u), false));
}

TEST_CASE(ShouldInstallRenderCodeDetourRejectsMissingOriginal)
{
    const shh::RenderHookTargets targets{};

    CHECK_TRUE(!shh::ShouldInstallRenderCodeDetour(
        targets, static_cast<std::uintptr_t>(0x30003000u), false));
}

TEST_CASE(ShouldInstallRenderCodeDetourRejectsExistingGateway)
{
    const shh::RenderHookTargets targets{
        .vtableOriginal = static_cast<std::uintptr_t>(0x10001000u),
        .codeGateway = static_cast<std::uintptr_t>(0x20002000u),
    };

    CHECK_TRUE(!shh::ShouldInstallRenderCodeDetour(
        targets, static_cast<std::uintptr_t>(0x30003000u), false));
}

TEST_CASE(ShouldInstallRenderCodeDetourRejectsReplacementLoop)
{
    const shh::RenderHookTargets targets{
        .vtableOriginal = static_cast<std::uintptr_t>(0x30003000u),
        .codeGateway = 0,
    };

    CHECK_TRUE(!shh::ShouldInstallRenderCodeDetour(
        targets, static_cast<std::uintptr_t>(0x30003000u), false));
}

TEST_CASE(ShouldInstallRenderCodeDetourRejectsPrepatchedTarget)
{
    const shh::RenderHookTargets targets{
        .vtableOriginal = static_cast<std::uintptr_t>(0x10001000u),
        .codeGateway = 0,
    };

    CHECK_TRUE(!shh::ShouldInstallRenderCodeDetour(
        targets, static_cast<std::uintptr_t>(0x30003000u), true));
}
