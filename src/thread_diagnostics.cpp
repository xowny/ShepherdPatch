#include "thread_diagnostics.h"

#include <cstdio>
#include <sstream>

namespace shh
{
namespace
{
std::string FormatHexAddress(std::uintptr_t value)
{
    char buffer[24] = {};
    std::snprintf(buffer, sizeof(buffer), "0x%08llX",
                  static_cast<unsigned long long>(value));
    return buffer;
}
} // namespace

std::string FormatModuleRelativeAddress(std::uintptr_t address, std::uintptr_t moduleBase,
                                        std::string_view moduleLabel)
{
    if (address == 0)
    {
        return "0x00000000";
    }

    std::ostringstream stream;
    stream << FormatHexAddress(address);
    if (moduleBase != 0 && address >= moduleBase)
    {
        stream << " (" << moduleLabel << "+0x" << std::hex << (address - moduleBase) << ')';
    }

    return stream.str();
}

LegacyThreadRole ClassifyLegacyThreadTag(std::string_view tagText)
{
    if (tagText == "LoadingScreenThread")
    {
        return LegacyThreadRole::LoadingScreen;
    }

    if (tagText == "StdAudio Engine")
    {
        return LegacyThreadRole::StdAudioEngine;
    }

    return LegacyThreadRole::Unknown;
}

std::string_view DescribeLegacyThreadRole(LegacyThreadRole role)
{
    switch (role)
    {
    case LegacyThreadRole::LoadingScreen:
        return "LoadingScreen";
    case LegacyThreadRole::StdAudioEngine:
        return "StdAudioEngine";
    case LegacyThreadRole::Unknown:
    default:
        return "Unknown";
    }
}

std::string DescribeLegacyThreadWrapperSnapshot(const LegacyThreadWrapperSnapshot& snapshot,
                                                std::uintptr_t engineModuleBase)
{
    std::ostringstream stream;
    stream << "payload.thread=" << FormatHexAddress(snapshot.threadHandleValue)
           << ", payload.entry="
           << FormatModuleRelativeAddress(snapshot.entryPoint, engineModuleBase, "g_SilentHill")
           << ", payload.parameter=" << FormatHexAddress(snapshot.userParameter)
           << ", payload.tag=" << FormatHexAddress(snapshot.tag)
           << ", payload.state=0x" << std::hex << static_cast<unsigned int>(snapshot.stateByte);
    return stream.str();
}

std::string DescribeLegacyThreadWrapperObjectGraphSnapshot(
    const LegacyThreadWrapperObjectGraphSnapshot& snapshot, std::uintptr_t engineModuleBase)
{
    std::ostringstream stream;
    stream << "payload.object="
           << FormatModuleRelativeAddress(snapshot.rootObject, engineModuleBase, "g_SilentHill")
           << ", payload.object.vtable="
           << FormatModuleRelativeAddress(snapshot.rootVtable, engineModuleBase, "g_SilentHill")
           << ", payload.object+0x1c="
           << FormatModuleRelativeAddress(snapshot.object1C, engineModuleBase, "g_SilentHill")
           << ", payload.object+0x1c.vtable="
           << FormatModuleRelativeAddress(snapshot.object1CVtable, engineModuleBase, "g_SilentHill")
           << ", payload.object+0x58="
           << FormatModuleRelativeAddress(snapshot.object58, engineModuleBase, "g_SilentHill")
           << ", payload.object+0x58.vtable="
           << FormatModuleRelativeAddress(snapshot.object58Vtable, engineModuleBase, "g_SilentHill")
           << ", payload.object+0x1c+0xb4="
           << FormatModuleRelativeAddress(snapshot.nestedB4, engineModuleBase, "g_SilentHill")
           << ", payload.object+0x1c+0xb4.vtable="
           << FormatModuleRelativeAddress(snapshot.nestedB4Vtable, engineModuleBase,
                                          "g_SilentHill");
    return stream.str();
}
} // namespace shh
