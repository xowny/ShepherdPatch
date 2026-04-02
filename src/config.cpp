#include "config.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <string>

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

bool ParseBoolValue(std::string_view value, bool currentValue)
{
    if (EqualsIgnoreCase(value, "true") || value == "1")
    {
        return true;
    }

    if (EqualsIgnoreCase(value, "false") || value == "0")
    {
        return false;
    }

    return currentValue;
}

std::uint32_t ParseUintValue(std::string_view value, std::uint32_t currentValue)
{
    std::uint32_t parsedValue = currentValue;
    const char* begin = value.data();
    const char* end = begin + value.size();
    const auto result = std::from_chars(begin, end, parsedValue);
    if (result.ec != std::errc{} || result.ptr != end)
    {
        return currentValue;
    }

    return parsedValue;
}

float ParseFloatValue(std::string_view value, float currentValue)
{
    std::string buffer(value);
    char* end = nullptr;
    const float parsedValue = std::strtof(buffer.c_str(), &end);
    if (end == buffer.c_str() || end == nullptr || *end != '\0')
    {
        return currentValue;
    }

    return parsedValue;
}
} // namespace

Config ParseConfig(std::string_view text)
{
    Config config;
    std::size_t cursor = 0;

    while (cursor < text.size())
    {
        const std::size_t lineEnd = text.find('\n', cursor);
        const std::size_t lineLength =
            lineEnd == std::string_view::npos ? text.size() - cursor : lineEnd - cursor;
        std::string line = Trim(text.substr(cursor, lineLength));
        cursor = lineEnd == std::string_view::npos ? text.size() : lineEnd + 1;

        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        const std::size_t separator = line.find('=');
        if (separator == std::string::npos)
        {
            continue;
        }

        const std::string key = Trim(std::string_view(line).substr(0, separator));
        const std::string value = Trim(std::string_view(line).substr(separator + 1));
        if (key.empty() || value.empty())
        {
            continue;
        }

        if (key == "EnableSafeResolutionChanges")
        {
            config.enableSafeResolutionChanges =
                ParseBoolValue(value, config.enableSafeResolutionChanges);
        }
        else if (key == "EnableDpiAwareness")
        {
            config.enableDpiAwareness = ParseBoolValue(value, config.enableDpiAwareness);
        }
        else if (key == "EnableUltrawideFovFix")
        {
            config.enableUltrawideFovFix =
                ParseBoolValue(value, config.enableUltrawideFovFix);
        }
        else if (key == "ForceBorderless")
        {
            config.forceBorderless = ParseBoolValue(value, config.forceBorderless);
        }
        else if (key == "RetryResetInWindowedMode")
        {
            config.retryResetInWindowedMode =
                ParseBoolValue(value, config.retryResetInWindowedMode);
        }
        else if (key == "EnableHighPrecisionTiming")
        {
            config.enableHighPrecisionTiming =
                ParseBoolValue(value, config.enableHighPrecisionTiming);
        }
        else if (key == "RequestHighResolutionTimer")
        {
            config.requestHighResolutionTimer =
                ParseBoolValue(value, config.requestHighResolutionTimer);
        }
        else if (key == "EnableRawMouseInput")
        {
            config.enableRawMouseInput =
                ParseBoolValue(value, config.enableRawMouseInput);
        }
        else if (key == "EnablePreciseSleepShim")
        {
            config.enablePreciseSleepShim =
                ParseBoolValue(value, config.enablePreciseSleepShim);
        }
        else if (key == "EnableCrashDumps")
        {
            config.enableCrashDumps =
                ParseBoolValue(value, config.enableCrashDumps);
        }
        else if (key == "EnableFrameRateUnlock")
        {
            config.enableFrameRateUnlock =
                ParseBoolValue(value, config.enableFrameRateUnlock);
        }
        else if (key == "RawMouseSensitivity")
        {
            config.rawMouseSensitivity =
                ParseFloatValue(value, config.rawMouseSensitivity);
        }
        else if (key == "InvertRawMouseY")
        {
            config.invertRawMouseY =
                ParseBoolValue(value, config.invertRawMouseY);
        }
        else if (key == "SynchronizeEngineVars")
        {
            config.synchronizeEngineVars =
                ParseBoolValue(value, config.synchronizeEngineVars);
        }
        else if (key == "EnableGamesMmcssProfile")
        {
            config.enableGamesMmcssProfile =
                ParseBoolValue(value, config.enableGamesMmcssProfile);
        }
        else if (key == "DisablePowerThrottling")
        {
            config.disablePowerThrottling =
                ParseBoolValue(value, config.disablePowerThrottling);
        }
        else if (key == "DisableVsync")
        {
            config.disableVsync =
                ParseBoolValue(value, config.disableVsync);
        }
        else if (key == "DisableLegacyPeriodicIconicTimer")
        {
            config.disableLegacyPeriodicIconicTimer =
                ParseBoolValue(value, config.disableLegacyPeriodicIconicTimer);
        }
        else if (key == "ReduceLegacyWaitableTimerPolling")
        {
            config.reduceLegacyWaitableTimerPolling =
                ParseBoolValue(value, config.reduceLegacyWaitableTimerPolling);
        }
        else if (key == "HardenLegacyGraphicsRecovery")
        {
            config.hardenLegacyGraphicsRecovery =
                ParseBoolValue(value, config.hardenLegacyGraphicsRecovery);
        }
        else if (key == "HardenLegacyThreadWrapper")
        {
            config.hardenLegacyThreadWrapper =
                ParseBoolValue(value, config.hardenLegacyThreadWrapper);
        }
        else if (key == "HardenDirectInputMouseDevice")
        {
            config.hardenDirectInputMouseDevice =
                ParseBoolValue(value, config.hardenDirectInputMouseDevice);
        }
        else if (key == "ReduceBorderlessPresentStutter")
        {
            config.reduceBorderlessPresentStutter =
                ParseBoolValue(value, config.reduceBorderlessPresentStutter);
        }
        else if (key == "ReduceMenuMovieStutter")
        {
            config.reduceMenuMovieStutter =
                ParseBoolValue(value, config.reduceMenuMovieStutter);
        }
        else if (key == "EnableFlipExSwapEffect")
        {
            config.enableFlipExSwapEffect =
                ParseBoolValue(value, config.enableFlipExSwapEffect);
        }
        else if (key == "LegacyGraphicsRetryDelayMilliseconds")
        {
            config.legacyGraphicsRetryDelayMilliseconds =
                ParseUintValue(value, config.legacyGraphicsRetryDelayMilliseconds);
        }
        else if (key == "LegacyThreadTerminateGraceMilliseconds")
        {
            config.legacyThreadTerminateGraceMilliseconds =
                ParseUintValue(value, config.legacyThreadTerminateGraceMilliseconds);
        }
        else if (key == "PresentWakeLeadMilliseconds")
        {
            config.presentWakeLeadMilliseconds =
                ParseFloatValue(value, config.presentWakeLeadMilliseconds);
        }
        else if (key == "TargetFrameRate")
        {
            config.targetFrameRate =
                ParseUintValue(value, config.targetFrameRate);
        }
        else if (key == "MaximumPresentationInterval")
        {
            config.maximumPresentationInterval =
                ParseUintValue(value, config.maximumPresentationInterval);
        }
        else if (key == "EnableHudViewportClamp")
        {
            config.enableHudViewportClamp =
                ParseBoolValue(value, config.enableHudViewportClamp);
        }
        else if (key == "HudViewportAspectRatio")
        {
            config.hudViewportAspectRatio =
                ParseFloatValue(value, config.hudViewportAspectRatio);
        }
        else if (key == "FallbackWidth")
        {
            config.fallbackWidth = ParseUintValue(value, config.fallbackWidth);
        }
        else if (key == "FallbackHeight")
        {
            config.fallbackHeight = ParseUintValue(value, config.fallbackHeight);
        }
        else if (key == "FallbackRefreshRate")
        {
            config.fallbackRefreshRate = ParseUintValue(value, config.fallbackRefreshRate);
        }
    }

    return config;
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
} // namespace shh
