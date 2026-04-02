#include "scheduler_targets.h"
#include "test_registry.h"

#include <array>

TEST_CASE(ResolveSchedulerLoopTargetsReturnsZeroesForNullObject)
{
    const shh::SchedulerLoopTargets targets = shh::ResolveSchedulerLoopTargets(nullptr);

    CHECK_EQ(targets.updateTarget, 0u);
    CHECK_EQ(targets.postUpdateTarget, 0u);
}

TEST_CASE(ResolveSchedulerLoopTargetsReturnsZeroesForNullVtable)
{
    const std::uintptr_t objectWords[1] = {0};

    const shh::SchedulerLoopTargets targets =
        shh::ResolveSchedulerLoopTargets(objectWords);

    CHECK_EQ(targets.updateTarget, 0u);
    CHECK_EQ(targets.postUpdateTarget, 0u);
}

TEST_CASE(ResolveSchedulerLoopTargetsReadsSchedulerVirtualMethods)
{
    std::array<std::uintptr_t, 168> vtable{};
    vtable[0x298 / sizeof(std::uintptr_t)] = 0x10AABBCC;
    vtable[0x29c / sizeof(std::uintptr_t)] = 0x10DDEEFF;

    const std::uintptr_t objectWords[1] = {
        reinterpret_cast<std::uintptr_t>(vtable.data()),
    };

    const shh::SchedulerLoopTargets targets =
        shh::ResolveSchedulerLoopTargets(objectWords);

    CHECK_EQ(targets.updateTarget, 0x10AABBCCu);
    CHECK_EQ(targets.postUpdateTarget, 0x10DDEEFFu);
}
