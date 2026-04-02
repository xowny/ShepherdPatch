#include "render_hook_targets.h"

namespace shh
{
std::uintptr_t ResolveRenderCallTarget(const RenderHookTargets& targets)
{
    return targets.codeGateway != 0 ? targets.codeGateway : targets.vtableOriginal;
}

bool ShouldInstallRenderCodeDetour(const RenderHookTargets& targets,
                                   std::uintptr_t replacementAddress,
                                   bool targetStartsWithRelativeJump)
{
    if (targets.vtableOriginal == 0 || replacementAddress == 0)
    {
        return false;
    }

    if (targets.codeGateway != 0 || targetStartsWithRelativeJump)
    {
        return false;
    }

    return targets.vtableOriginal != replacementAddress;
}
} // namespace shh
