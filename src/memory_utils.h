#pragma once

#include <cstddef>
#include <cstdint>

namespace shh
{
bool IsReadableMemoryRange(const void* address, std::size_t size);
bool IsExecutableMemoryAddress(std::uintptr_t address);
bool TryReadByteValue(const void* address, std::uint8_t& value);
bool TryReadPointerValue(const void* address, std::uintptr_t& value);
} // namespace shh
