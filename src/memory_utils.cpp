#include "memory_utils.h"

#include <windows.h>

namespace shh
{
namespace
{
bool IsReadableProtection(DWORD protection)
{
    switch (protection & 0xff)
    {
    case PAGE_READONLY:
    case PAGE_READWRITE:
    case PAGE_WRITECOPY:
    case PAGE_EXECUTE_READ:
    case PAGE_EXECUTE_READWRITE:
    case PAGE_EXECUTE_WRITECOPY:
        return true;
    default:
        return false;
    }
}

bool IsExecutableProtection(DWORD protection)
{
    switch (protection & 0xff)
    {
    case PAGE_EXECUTE:
    case PAGE_EXECUTE_READ:
    case PAGE_EXECUTE_READWRITE:
    case PAGE_EXECUTE_WRITECOPY:
        return true;
    default:
        return false;
    }
}
} // namespace

bool IsReadableMemoryRange(const void* address, std::size_t size)
{
    if (address == nullptr || size == 0)
    {
        return false;
    }

    MEMORY_BASIC_INFORMATION memoryInfo = {};
    if (VirtualQuery(address, &memoryInfo, sizeof(memoryInfo)) == 0 ||
        memoryInfo.State != MEM_COMMIT || (memoryInfo.Protect & PAGE_GUARD) != 0 ||
        !IsReadableProtection(memoryInfo.Protect))
    {
        return false;
    }

    const auto begin = reinterpret_cast<std::uintptr_t>(address);
    const auto end = begin + size;
    const auto regionBegin = reinterpret_cast<std::uintptr_t>(memoryInfo.BaseAddress);
    const auto regionEnd = regionBegin + memoryInfo.RegionSize;
    return begin >= regionBegin && end <= regionEnd;
}

bool IsExecutableMemoryAddress(std::uintptr_t address)
{
    if (address == 0)
    {
        return false;
    }

    MEMORY_BASIC_INFORMATION memoryInfo = {};
    if (VirtualQuery(reinterpret_cast<const void*>(address), &memoryInfo, sizeof(memoryInfo)) ==
            0 ||
        memoryInfo.State != MEM_COMMIT || (memoryInfo.Protect & PAGE_GUARD) != 0)
    {
        return false;
    }

    return IsExecutableProtection(memoryInfo.Protect);
}

bool TryReadByteValue(const void* address, std::uint8_t& value)
{
    value = 0;
    if (!IsReadableMemoryRange(address, sizeof(std::uint8_t)))
    {
        return false;
    }

    __try
    {
        value = *reinterpret_cast<const std::uint8_t*>(address);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        value = 0;
        return false;
    }
}

bool TryReadPointerValue(const void* address, std::uintptr_t& value)
{
    value = 0;
    if (!IsReadableMemoryRange(address, sizeof(std::uintptr_t)))
    {
        return false;
    }

    __try
    {
        value = *reinterpret_cast<const std::uintptr_t*>(address);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        value = 0;
        return false;
    }
}
} // namespace shh
