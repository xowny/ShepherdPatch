#include "intro_skip.h"

#include <array>

namespace shh
{
namespace
{
constexpr std::string_view kStartLoopMovie = "start_loop.bik";
constexpr std::string_view kAttractMovie = "shh_attract.bik";

struct StartupLogoTimerByte
{
    std::uint64_t offset;
    std::uint8_t expected;
};

constexpr std::array<StartupLogoTimerByte, 16> kStartupLogoTimerBytes{{
    {0x579E30F, '1'}, {0x579E404, '1'}, {0x579E473, '2'}, {0x579E529, '1'},
    {0x579E5DC, '1'}, {0x579E6FE, '1'}, {0x579E777, '2'}, {0x579E848, '1'},
    {0x579E916, '1'}, {0x579E9CE, '1'}, {0x579EA66, '1'}, {0x579EAF2, '4'},
    {0x579EBA7, '1'}, {0x579EC24, '1'}, {0x579ECF0, '2'}, {0x579EDD4, '1'},
}};

char ToLowerAscii(char value)
{
    if (value >= 'A' && value <= 'Z')
    {
        return static_cast<char>(value - 'A' + 'a');
    }

    return value;
}

bool EqualsIgnoreCaseAscii(std::string_view lhs, std::string_view rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }

    for (std::size_t index = 0; index < lhs.size(); ++index)
    {
        if (ToLowerAscii(lhs[index]) != ToLowerAscii(rhs[index]))
        {
            return false;
        }
    }

    return true;
}

std::string_view BaseName(std::string_view path)
{
    const std::size_t separator = path.find_last_of("/\\");
    if (separator == std::string_view::npos)
    {
        return path;
    }

    return path.substr(separator + 1);
}
} // namespace

bool ShouldApplyMenuMovieWaitFix(std::string_view moviePath)
{
    const std::string_view baseName = BaseName(moviePath);
    return EqualsIgnoreCaseAscii(baseName, kStartLoopMovie) ||
           EqualsIgnoreCaseAscii(baseName, kAttractMovie);
}

MenuMovieWaitPlan BuildMenuMovieWaitPlan(bool reduceMenuMovieStutter,
                                         std::string_view moviePath)
{
    if (!reduceMenuMovieStutter || !ShouldApplyMenuMovieWaitFix(moviePath))
    {
        return {};
    }

    return MenuMovieWaitPlan{
        .trackHandle = true,
        .forceZeroWait = true,
    };
}

bool IsSupportedGlobalPakPath(std::string_view path, std::uint64_t fileSize)
{
    return fileSize == kSupportedGlobalPakSize &&
           EqualsIgnoreCaseAscii(BaseName(path), "GLOBAL.PAK");
}

StartupLogoPatchResult PatchStartupLogoTimers(std::uint64_t fileOffset,
                                              std::span<std::uint8_t> bytes)
{
    StartupLogoPatchResult result;
    const std::uint64_t endOffset = fileOffset + bytes.size();
    if (endOffset < fileOffset)
    {
        return result;
    }

    for (const StartupLogoTimerByte timer : kStartupLogoTimerBytes)
    {
        if (timer.offset < fileOffset || timer.offset >= endOffset)
        {
            continue;
        }

        std::uint8_t& value = bytes[static_cast<std::size_t>(timer.offset - fileOffset)];
        if (value != timer.expected)
        {
            ++result.mismatchedBytes;
            continue;
        }

        value = '0';
        ++result.patchedBytes;
    }
    return result;
}
} // namespace shh
