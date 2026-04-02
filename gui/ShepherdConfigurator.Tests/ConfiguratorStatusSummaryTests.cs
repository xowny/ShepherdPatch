using ShepherdConfigurator.Shell;

namespace ShepherdConfigurator.Tests;

public sealed class ConfiguratorStatusSummaryTests
{
    [Fact]
    public void IncludesExperimentalCountWhenPresent()
    {
        string summary = ConfiguratorStatusSummary.Build(
            visibleSectionCount: 2,
            selectedCategory: "All",
            query: "frame",
            isDirty: true,
            visibleExperimentalCount: 3);

        Assert.Contains("2 sections visible", summary);
        Assert.Contains("filter: \"frame\"", summary);
        Assert.Contains("unsaved changes", summary);
        Assert.Contains("3 experimental options in view", summary);
    }

    [Fact]
    public void OmitsExperimentalSuffixWhenNoneVisible()
    {
        string summary = ConfiguratorStatusSummary.Build(
            visibleSectionCount: 1,
            selectedCategory: "Display",
            query: "",
            isDirty: false,
            visibleExperimentalCount: 0);

        Assert.Equal("Display view • no filter • saved state", summary);
    }
}
