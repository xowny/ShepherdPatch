using ShepherdConfigurator.Services;

namespace ShepherdConfigurator.Tests;

public sealed class RuntimePrerequisiteAnalyzerTests
{
    [Fact]
    public void GetMissingPrerequisitesReturnsOnlyRequiredMissingEntries()
    {
        RuntimePrerequisite requiredMissing = new(
            Id: "required-missing",
            DisplayName: "Required Missing",
            Description: "Required and not installed.",
            DownloadUrl: "https://example.invalid/required-missing",
            IsRequired: true,
            IsInstalled: static () => false);

        RuntimePrerequisite requiredInstalled = new(
            Id: "required-installed",
            DisplayName: "Required Installed",
            Description: "Required and installed.",
            DownloadUrl: "https://example.invalid/required-installed",
            IsRequired: true,
            IsInstalled: static () => true);

        RuntimePrerequisite optionalMissing = new(
            Id: "optional-missing",
            DisplayName: "Optional Missing",
            Description: "Optional and not installed.",
            DownloadUrl: "https://example.invalid/optional-missing",
            IsRequired: false,
            IsInstalled: static () => false);

        IReadOnlyList<RuntimePrerequisite> missing = RuntimePrerequisiteAnalyzer.GetMissingPrerequisites(
            [requiredMissing, requiredInstalled, optionalMissing]);

        Assert.Single(missing);
        Assert.Equal(requiredMissing, missing[0]);
    }

    [Fact]
    public void GetMissingPrerequisitesReturnsEmptyForCurrentPackagingProfile()
    {
        IReadOnlyList<RuntimePrerequisite> missing = RuntimePrerequisiteAnalyzer.GetMissingPrerequisites();

        Assert.Empty(missing);
    }
}
