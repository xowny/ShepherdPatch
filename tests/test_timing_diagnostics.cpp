#include "test_registry.h"
#include "timing_diagnostics.h"

TEST_CASE(IsSuspiciousFramePacingIntervalMsMatchesFrameCapLikePeriods)
{
    CHECK_TRUE(shh::IsSuspiciousFramePacingIntervalMs(15));
    CHECK_TRUE(shh::IsSuspiciousFramePacingIntervalMs(16));
    CHECK_TRUE(shh::IsSuspiciousFramePacingIntervalMs(33));
    CHECK_TRUE(shh::IsSuspiciousFramePacingIntervalMs(40));
    CHECK_TRUE(!shh::IsSuspiciousFramePacingIntervalMs(14));
    CHECK_TRUE(!shh::IsSuspiciousFramePacingIntervalMs(41));
}

TEST_CASE(RelativeDueTimeToMillisecondsOnlyConvertsRelativeTimers)
{
    const auto thirtyThreeMs = shh::RelativeDueTimeToMilliseconds(-330000);
    CHECK_TRUE(thirtyThreeMs.has_value());
    CHECK_EQ(*thirtyThreeMs, 33u);

    const auto sixteenPointSevenMs = shh::RelativeDueTimeToMilliseconds(-166667);
    CHECK_TRUE(sixteenPointSevenMs.has_value());
    CHECK_EQ(*sixteenPointSevenMs, 17u);

    const auto absoluteTimer = shh::RelativeDueTimeToMilliseconds(330000);
    CHECK_TRUE(!absoluteTimer.has_value());
}

TEST_CASE(IsCadenceOutlierUsesTargetScaledThresholdWithMinimumFloor)
{
    CHECK_TRUE(!shh::IsCadenceOutlier(21.0, 16.6667));
    CHECK_TRUE(shh::IsCadenceOutlier(23.0, 16.6667));
    CHECK_TRUE(!shh::IsCadenceOutlier(19.5, 8.3333));
    CHECK_TRUE(shh::IsCadenceOutlier(20.0, 8.3333));
}

TEST_CASE(ConsumeCadenceSummaryAggregatesAndResetsTimingWindow)
{
    shh::CadenceSummaryWindow window{};

    shh::AccumulateCadenceSummary(&window, 16.0, 16.6667);
    shh::AccumulateCadenceSummary(&window, 24.0, 16.6667);
    const auto beforeInterval = shh::ConsumeCadenceSummary(&window, 3);
    CHECK_TRUE(!beforeInterval.has_value());

    shh::AccumulateCadenceSummary(&window, 20.0, 16.6667);
    const auto summary = shh::ConsumeCadenceSummary(&window, 3);
    CHECK_TRUE(summary.has_value());
    CHECK_EQ(summary->sampleCount, 3u);
    CHECK_EQ(summary->stutterCount, 1u);
    CHECK_TRUE(summary->averageMilliseconds > 19.9 && summary->averageMilliseconds < 20.1);
    CHECK_EQ(summary->minMilliseconds, 16.0);
    CHECK_EQ(summary->maxMilliseconds, 24.0);

    CHECK_EQ(window.sampleCount, 0u);
    CHECK_EQ(window.stutterCount, 0u);
    CHECK_EQ(window.totalMilliseconds, 0.0);
    CHECK_EQ(window.minMilliseconds, 0.0);
    CHECK_EQ(window.maxMilliseconds, 0.0);
}
