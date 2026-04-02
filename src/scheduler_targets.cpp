#include "scheduler_targets.h"

#include <cstddef>

namespace shh
{
namespace
{
constexpr std::size_t kSchedulerUpdateVtableOffset = 0x298 / sizeof(std::uintptr_t);
constexpr std::size_t kSchedulerPostUpdateVtableOffset = 0x29c / sizeof(std::uintptr_t);
} // namespace

SchedulerLoopTargets ResolveSchedulerLoopTargets(const void* objectPointer)
{
    if (objectPointer == nullptr)
    {
        return {};
    }

    const auto objectWords = static_cast<const std::uintptr_t*>(objectPointer);
    const auto vtableAddress = objectWords[0];
    if (vtableAddress == 0)
    {
        return {};
    }

    const auto vtable = reinterpret_cast<const std::uintptr_t*>(vtableAddress);
    return SchedulerLoopTargets{
        .updateTarget = vtable[kSchedulerUpdateVtableOffset],
        .postUpdateTarget = vtable[kSchedulerPostUpdateVtableOffset],
    };
}
} // namespace shh
