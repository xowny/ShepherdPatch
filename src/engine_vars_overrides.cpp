#include "engine_vars_overrides.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace shh
{
namespace
{
bool EqualsIgnoreCase(std::string_view lhs, std::string_view rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }

    for (std::size_t index = 0; index < lhs.size(); ++index)
    {
        const unsigned char left = static_cast<unsigned char>(lhs[index]);
        const unsigned char right = static_cast<unsigned char>(rhs[index]);
        if (std::tolower(left) != std::tolower(right))
        {
            return false;
        }
    }

    return true;
}

std::string Trim(std::string_view value)
{
    std::size_t begin = 0;
    while (begin < value.size() &&
           std::isspace(static_cast<unsigned char>(value[begin])) != 0)
    {
        ++begin;
    }

    std::size_t end = value.size();
    while (end > begin &&
           std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
    {
        --end;
    }

    return std::string(value.substr(begin, end - begin));
}

std::string BoolValue(bool value)
{
    return value ? "true" : "false";
}

std::string DetectNewline(std::string_view text)
{
    return text.find("\r\n") != std::string_view::npos ? "\r\n" : "\n";
}

std::vector<std::string> SplitLines(std::string_view text)
{
    std::vector<std::string> lines;
    std::size_t cursor = 0;
    while (cursor < text.size())
    {
        const std::size_t lineEnd = text.find('\n', cursor);
        const std::size_t lineLength =
            lineEnd == std::string_view::npos ? text.size() - cursor : lineEnd - cursor;
        std::string line(text.substr(cursor, lineLength));
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        lines.push_back(std::move(line));
        cursor = lineEnd == std::string_view::npos ? text.size() : lineEnd + 1;
    }

    if (text.empty())
    {
        lines.emplace_back();
    }

    return lines;
}

bool ApplyOverride(std::vector<std::string>& lines, std::string_view key, std::string_view value)
{
    const std::string replacement = std::string(key) + "=" + std::string(value);
    for (std::string& line : lines)
    {
        const std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == '#')
        {
            continue;
        }

        const std::size_t separator = trimmed.find('=');
        if (separator == std::string::npos)
        {
            continue;
        }

        if (!EqualsIgnoreCase(Trim(std::string_view(trimmed).substr(0, separator)), key))
        {
            continue;
        }

        if (trimmed == replacement)
        {
            return false;
        }

        line = replacement;
        return true;
    }

    lines.push_back(replacement);
    return true;
}
} // namespace

std::string ApplyEngineVarsOverrides(std::string_view text,
                                     const EngineVarsOverrides& overrides)
{
    const std::string newline = DetectNewline(text);
    std::vector<std::string> lines = SplitLines(text);
    bool changed = false;

    if (overrides.syncFullscreenToBorderless)
    {
        changed |= ApplyOverride(lines, "FullScreen", BoolValue(false));
    }

    if (overrides.screenRefresh.has_value())
    {
        changed |= ApplyOverride(lines, "ScreenRefresh",
                                 std::to_string(*overrides.screenRefresh));
    }

    if (!changed)
    {
        return std::string(text);
    }

    std::string result;
    const bool hadTrailingNewline =
        text.ends_with('\n') || text.ends_with('\r');
    for (std::size_t index = 0; index < lines.size(); ++index)
    {
        result += lines[index];
        if (index + 1 < lines.size() || hadTrailingNewline)
        {
            result += newline;
        }
    }

    return result;
}
} // namespace shh
