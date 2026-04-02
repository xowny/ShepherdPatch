#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace shh
{
enum class LegacyThreadRole
{
    Unknown,
    LoadingScreen,
    StdAudioEngine,
};

struct LegacyThreadWrapperSnapshot
{
    std::uintptr_t threadHandleValue = 0;
    std::uintptr_t entryPoint = 0;
    std::uintptr_t userParameter = 0;
    std::uintptr_t tag = 0;
    std::uint8_t stateByte = 0;
};

struct LegacyThreadWrapperObjectGraphSnapshot
{
    std::uintptr_t rootObject = 0;
    std::uintptr_t rootVtable = 0;
    std::uintptr_t object1C = 0;
    std::uintptr_t object1CVtable = 0;
    std::uintptr_t object58 = 0;
    std::uintptr_t object58Vtable = 0;
    std::uintptr_t nestedB4 = 0;
    std::uintptr_t nestedB4Vtable = 0;
};

std::string FormatModuleRelativeAddress(std::uintptr_t address, std::uintptr_t moduleBase,
                                        std::string_view moduleLabel);
LegacyThreadRole ClassifyLegacyThreadTag(std::string_view tagText);
std::string_view DescribeLegacyThreadRole(LegacyThreadRole role);

std::string DescribeLegacyThreadWrapperSnapshot(const LegacyThreadWrapperSnapshot& snapshot,
                                                std::uintptr_t engineModuleBase);

std::string DescribeLegacyThreadWrapperObjectGraphSnapshot(
    const LegacyThreadWrapperObjectGraphSnapshot& snapshot, std::uintptr_t engineModuleBase);
} // namespace shh
