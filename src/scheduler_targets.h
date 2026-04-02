#pragma once

#include <cstdint>

namespace shh
{
struct SchedulerLoopTargets
{
    std::uintptr_t updateTarget = 0;
    std::uintptr_t postUpdateTarget = 0;
};

SchedulerLoopTargets ResolveSchedulerLoopTargets(const void* objectPointer);
} // namespace shh
