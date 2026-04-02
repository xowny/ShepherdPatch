#pragma once

#include <cstdint>

namespace shh
{
struct RenderHookTargets
{
    std::uintptr_t vtableOriginal = 0;
    std::uintptr_t codeGateway = 0;
};

std::uintptr_t ResolveRenderCallTarget(const RenderHookTargets& targets);
bool ShouldInstallRenderCodeDetour(const RenderHookTargets& targets,
                                   std::uintptr_t replacementAddress,
                                   bool targetStartsWithRelativeJump);
} // namespace shh
