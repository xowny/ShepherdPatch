#include "memory_utils.h"
#include "test_registry.h"

#include <windows.h>

#include <cstdint>
#include <stdexcept>

namespace
{
class ScopedVirtualAlloc
{
public:
    explicit ScopedVirtualAlloc(DWORD protection)
        : memory_(::VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, protection))
    {
        if (memory_ == nullptr)
        {
            throw std::runtime_error("VirtualAlloc failed");
        }
    }

    ~ScopedVirtualAlloc()
    {
        if (memory_ != nullptr)
        {
            ::VirtualFree(memory_, 0, MEM_RELEASE);
        }
    }

    void* get() const
    {
        return memory_;
    }

private:
    void* memory_ = nullptr;
};
} // namespace

TEST_CASE(IsReadableMemoryRangeAcceptsReadWritePages)
{
    ScopedVirtualAlloc memory(PAGE_READWRITE);
    CHECK_TRUE(shh::IsReadableMemoryRange(memory.get(), sizeof(std::uintptr_t)));
}

TEST_CASE(IsReadableMemoryRangeRejectsExecuteOnlyPages)
{
    ScopedVirtualAlloc memory(PAGE_EXECUTE);
    CHECK_TRUE(!shh::IsReadableMemoryRange(memory.get(), sizeof(std::uintptr_t)));
}

TEST_CASE(TryReadPointerValueReadsReadablePages)
{
    ScopedVirtualAlloc memory(PAGE_READWRITE);
    auto* valuePointer = static_cast<std::uintptr_t*>(memory.get());
    *valuePointer = static_cast<std::uintptr_t>(0x12345678u);

    std::uintptr_t value = 0;
    CHECK_TRUE(shh::TryReadPointerValue(valuePointer, value));
    CHECK_EQ(value, static_cast<std::uintptr_t>(0x12345678u));
}

TEST_CASE(TryReadByteValueReadsReadablePages)
{
    ScopedVirtualAlloc memory(PAGE_READWRITE);
    auto* valuePointer = static_cast<std::uint8_t*>(memory.get());
    *valuePointer = static_cast<std::uint8_t>(0xE9u);

    std::uint8_t value = 0;
    CHECK_TRUE(shh::TryReadByteValue(valuePointer, value));
    CHECK_EQ(value, static_cast<std::uint8_t>(0xE9u));
}

TEST_CASE(TryReadPointerValueRejectsExecuteOnlyPages)
{
    ScopedVirtualAlloc memory(PAGE_EXECUTE);

    std::uintptr_t value = static_cast<std::uintptr_t>(0xFFFFFFFFu);
    CHECK_TRUE(!shh::TryReadPointerValue(memory.get(), value));
    CHECK_EQ(value, static_cast<std::uintptr_t>(0u));
}

TEST_CASE(TryReadByteValueRejectsExecuteOnlyPages)
{
    ScopedVirtualAlloc memory(PAGE_EXECUTE);

    std::uint8_t value = 0xFFu;
    CHECK_TRUE(!shh::TryReadByteValue(memory.get(), value));
    CHECK_EQ(value, static_cast<std::uint8_t>(0u));
}
