#include "intro_skip.h"

namespace shh
{
namespace
{
constexpr std::string_view kStartLoopMovie = "start_loop.bik";
constexpr std::string_view kAttractMovie = "shh_attract.bik";

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
} // namespace shh
