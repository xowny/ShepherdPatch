#pragma once

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace shh::test
{
struct TestCase
{
    const char* name;
    void (*fn)();
};

inline std::vector<TestCase>& Registry()
{
    static std::vector<TestCase> tests;
    return tests;
}

struct Register
{
    Register(const char* name, void (*fn)())
    {
        Registry().push_back({name, fn});
    }
};
} // namespace shh::test

#define TEST_CASE(name)                                                       \
    void name();                                                              \
    namespace                                                                 \
    {                                                                         \
    shh::test::Register reg_##name(#name, &name);                             \
    }                                                                         \
    void name()

#define CHECK_TRUE(expr)                                                      \
    do                                                                        \
    {                                                                         \
        if (!(expr))                                                          \
        {                                                                     \
            throw std::runtime_error("CHECK_TRUE failed: " #expr);            \
        }                                                                     \
    } while (false)

#define CHECK_EQ(actual, expected)                                            \
    do                                                                        \
    {                                                                         \
        if (!((actual) == (expected)))                                        \
        {                                                                     \
            throw std::runtime_error("CHECK_EQ failed: " #actual " != " #expected); \
        }                                                                     \
    } while (false)
