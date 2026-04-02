using ShepherdConfigurator.Services;

namespace ShepherdConfigurator.Tests;

public sealed class ModDependencyServiceTests
{
    [Fact]
    public void ResolveInstallDirectoryUsesConfigDirectoryWhenItLooksLikeGameBin()
    {
        string tempRoot = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N"));
        string binDirectory = Path.Combine(tempRoot, "Bin");
        Directory.CreateDirectory(binDirectory);
        File.WriteAllText(Path.Combine(binDirectory, "SilentHill.exe"), string.Empty);

        string configPath = Path.Combine(binDirectory, "ShepherdPatch.ini");
        string path = ModDependencyAnalyzer.ResolveInstallDirectory(configPath);

        Assert.Equal(binDirectory, path);

        Directory.Delete(tempRoot, recursive: true);
    }

    [Fact]
    public void ResolveInstallDirectoryIgnoresRepoStyleConfigPath()
    {
        string path = ModDependencyAnalyzer.ResolveInstallDirectory(@"C:\ws\shh\ShepherdPatch.ini");

        Assert.Contains(@"Silent Hill Homecoming\Bin", path);
    }

    [Fact]
    public void ResolveInstallDirectoryFallsBackToSteamBinPath()
    {
        string path = ModDependencyAnalyzer.ResolveInstallDirectory(null);

        Assert.Contains(@"Silent Hill Homecoming\Bin", path);
    }

    [Fact]
    public void DependencyStatusReportsMissingFiles()
    {
        DependencyStatus status = new(
            InstallDirectory: @"C:\Games\SHH\Bin",
            MissingFiles: ["ShepherdPatch.asi", "version.dll"],
            BundledSourceDirectory: @"C:\Package");

        Assert.True(status.HasMissingDependencies);
        Assert.Equal(2, status.MissingFiles.Count);
    }
}
