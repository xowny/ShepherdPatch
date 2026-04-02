#include "crash_dump.h"

#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>

namespace shh
{
namespace
{
std::string SanitizeProcessName(std::string_view processName)
{
    std::string sanitized;
    sanitized.reserve(processName.size());

    bool previousWasSeparator = false;
    for (const char ch : processName)
    {
        const unsigned char current = static_cast<unsigned char>(ch);
        if (std::isalnum(current) != 0)
        {
            sanitized.push_back(static_cast<char>(current));
            previousWasSeparator = false;
            continue;
        }

        if (ch == '-' || ch == '_')
        {
            sanitized.push_back(ch);
            previousWasSeparator = false;
            continue;
        }

        if (!previousWasSeparator)
        {
            sanitized.push_back('_');
            previousWasSeparator = true;
        }
    }

    while (!sanitized.empty() && sanitized.front() == '_')
    {
        sanitized.erase(sanitized.begin());
    }

    while (!sanitized.empty() && sanitized.back() == '_')
    {
        sanitized.pop_back();
    }

    if (sanitized.empty())
    {
        return "ShepherdPatch";
    }

    return sanitized;
}
} // namespace

std::filesystem::path BuildCrashDumpPath(const std::filesystem::path& directory,
                                         std::string_view processName,
                                         const CrashDumpTimestamp& timestamp,
                                         std::uint32_t processId)
{
    std::ostringstream fileName;
    fileName << SanitizeProcessName(processName) << "-crash-" << std::setfill('0')
             << std::setw(4) << timestamp.year << std::setw(2) << timestamp.month
             << std::setw(2) << timestamp.day << '-' << std::setw(2) << timestamp.hour
             << std::setw(2) << timestamp.minute << std::setw(2) << timestamp.second << '-'
             << std::setw(3) << timestamp.milliseconds << "-pid" << processId << ".dmp";
    return directory / fileName.str();
}
} // namespace shh
