#pragma once

#include <cstdint>
#include <optional>

namespace shh
{
struct CadenceSummaryWindow
{
    std::uint32_t sampleCount = 0;
    std::uint32_t stutterCount = 0;
    double totalMilliseconds = 0.0;
    double minMilliseconds = 0.0;
    double maxMilliseconds = 0.0;
};

struct CadenceSummary
{
    std::uint32_t sampleCount = 0;
    std::uint32_t stutterCount = 0;
    double averageMilliseconds = 0.0;
    double minMilliseconds = 0.0;
    double maxMilliseconds = 0.0;
};

bool IsSuspiciousFramePacingIntervalMs(std::uint32_t milliseconds);
bool IsCadenceOutlier(double sampleMilliseconds, double targetMilliseconds);
void AccumulateCadenceSummary(CadenceSummaryWindow* window, double sampleMilliseconds,
                              double targetMilliseconds);
std::optional<CadenceSummary> ConsumeCadenceSummary(CadenceSummaryWindow* window,
                                                    std::uint32_t intervalSamples);
std::optional<std::uint32_t> RelativeDueTimeToMilliseconds(std::int64_t dueTimeHundredNanoseconds);
} // namespace shh
