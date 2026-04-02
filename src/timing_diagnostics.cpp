#include "timing_diagnostics.h"

#include <algorithm>

namespace shh
{
namespace
{
constexpr double kCadenceOutlierFactor = 1.35;
constexpr double kMinimumCadenceOutlierMilliseconds = 20.0;
}

bool IsSuspiciousFramePacingIntervalMs(std::uint32_t milliseconds)
{
    return milliseconds >= 15 && milliseconds <= 40;
}

bool IsCadenceOutlier(double sampleMilliseconds, double targetMilliseconds)
{
    if (sampleMilliseconds < 0.0 || targetMilliseconds <= 0.0)
    {
        return false;
    }

    const double thresholdMilliseconds =
        std::max(kMinimumCadenceOutlierMilliseconds, targetMilliseconds * kCadenceOutlierFactor);
    return sampleMilliseconds >= thresholdMilliseconds;
}

void AccumulateCadenceSummary(CadenceSummaryWindow* window, double sampleMilliseconds,
                              double targetMilliseconds)
{
    if (window == nullptr || sampleMilliseconds < 0.0)
    {
        return;
    }

    window->sampleCount += 1;
    window->totalMilliseconds += sampleMilliseconds;
    if (window->sampleCount == 1)
    {
        window->minMilliseconds = sampleMilliseconds;
        window->maxMilliseconds = sampleMilliseconds;
    }
    else
    {
        window->minMilliseconds = std::min(window->minMilliseconds, sampleMilliseconds);
        window->maxMilliseconds = std::max(window->maxMilliseconds, sampleMilliseconds);
    }

    if (IsCadenceOutlier(sampleMilliseconds, targetMilliseconds))
    {
        window->stutterCount += 1;
    }
}

std::optional<CadenceSummary> ConsumeCadenceSummary(CadenceSummaryWindow* window,
                                                    std::uint32_t intervalSamples)
{
    if (window == nullptr || intervalSamples == 0 || window->sampleCount < intervalSamples)
    {
        return std::nullopt;
    }

    const CadenceSummary summary{
        .sampleCount = window->sampleCount,
        .stutterCount = window->stutterCount,
        .averageMilliseconds =
            window->sampleCount > 0
                ? window->totalMilliseconds / static_cast<double>(window->sampleCount)
                : 0.0,
        .minMilliseconds = window->minMilliseconds,
        .maxMilliseconds = window->maxMilliseconds,
    };

    *window = {};
    return summary;
}

std::optional<std::uint32_t> RelativeDueTimeToMilliseconds(
    std::int64_t dueTimeHundredNanoseconds)
{
    if (dueTimeHundredNanoseconds >= 0)
    {
        return std::nullopt;
    }

    const std::uint64_t absoluteHundredNanoseconds =
        static_cast<std::uint64_t>(-dueTimeHundredNanoseconds);
    return static_cast<std::uint32_t>((absoluteHundredNanoseconds + 9999ULL) / 10000ULL);
}
} // namespace shh
