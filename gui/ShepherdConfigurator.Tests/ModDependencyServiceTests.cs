using ShepherdConfigurator.Services;

namespace ShepherdConfigurator.Tests;

public sealed class ModDependencyServiceTests
{
    [Fact]
    public void GameLaunchPlannerUsesSteamForARecognizedSteamLibrary()
    {
        string tempRoot = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N"));
        string steamAppsDirectory = Path.Combine(tempRoot, "steamapps");
        string binDirectory = Path.Combine(
            steamAppsDirectory, "common", "Silent Hill Homecoming", "Bin");
        Directory.CreateDirectory(binDirectory);
        File.WriteAllText(Path.Combine(binDirectory, "SilentHill.exe"), string.Empty);
        File.WriteAllText(
            Path.Combine(steamAppsDirectory, "appmanifest_19000.acf"),
            "\"AppState\" { \"appid\" \"19000\" }");

        GameLaunchPlan? plan = GameLaunchPlanner.Create(binDirectory, launchThroughSteam: true);

        Assert.NotNull(plan);
        Assert.Equal(GameLaunchKind.Steam, plan.Kind);
        Assert.Equal("steam://run/19000", plan.FileName);
        Assert.Null(plan.WorkingDirectory);

        Directory.Delete(tempRoot, recursive: true);
    }

    [Fact]
    public void GameLaunchPlannerKeepsDirectLaunchAsDefaultForSteamInstallation()
    {
        string tempRoot = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N"));
        string steamAppsDirectory = Path.Combine(tempRoot, "steamapps");
        string binDirectory = Path.Combine(
            steamAppsDirectory, "common", "Silent Hill Homecoming", "Bin");
        Directory.CreateDirectory(binDirectory);
        string executablePath = Path.Combine(binDirectory, "SilentHill.exe");
        File.WriteAllText(executablePath, string.Empty);
        File.WriteAllText(
            Path.Combine(steamAppsDirectory, "appmanifest_19000.acf"),
            "\"AppState\" { \"appid\" \"19000\" }");

        GameLaunchPlan? plan = GameLaunchPlanner.Create(binDirectory);

        Assert.NotNull(plan);
        Assert.Equal(GameLaunchKind.Executable, plan.Kind);
        Assert.Equal(executablePath, plan.FileName);

        Directory.Delete(tempRoot, recursive: true);
    }

    [Fact]
    public void GameLaunchPlannerUsesExecutableOutsideSteamLibrary()
    {
        string tempRoot = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N"));
        string binDirectory = Path.Combine(tempRoot, "Game", "Bin");
        Directory.CreateDirectory(binDirectory);
        string executablePath = Path.Combine(binDirectory, "SilentHill.exe");
        File.WriteAllText(executablePath, string.Empty);

        GameLaunchPlan? plan = GameLaunchPlanner.Create(binDirectory);

        Assert.NotNull(plan);
        Assert.Equal(GameLaunchKind.Executable, plan.Kind);
        Assert.Equal(executablePath, plan.FileName);
        Assert.Equal(binDirectory, plan.WorkingDirectory);

        Directory.Delete(tempRoot, recursive: true);
    }

    [Fact]
    public void GameLaunchPlannerDoesNotSilentlyFallBackWhenSteamWasRequested()
    {
        string tempRoot = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N"));
        string binDirectory = Path.Combine(tempRoot, "Game", "Bin");
        Directory.CreateDirectory(binDirectory);
        File.WriteAllText(Path.Combine(binDirectory, "SilentHill.exe"), string.Empty);

        Assert.Null(GameLaunchPlanner.Create(binDirectory, launchThroughSteam: true));

        Directory.Delete(tempRoot, recursive: true);
    }

    [Fact]
    public void GameLaunchPlannerRejectsDirectoryWithoutGameExecutable()
    {
        string tempRoot = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(tempRoot);

        Assert.Null(GameLaunchPlanner.Create(tempRoot));

        Directory.Delete(tempRoot, recursive: true);
    }

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

        Assert.True(string.IsNullOrEmpty(path) || Directory.Exists(path));
    }

    [Fact]
    public void ResolveInstallDirectoryUsesSelectedGameBinPath()
    {
        string tempRoot = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N"));
        string binDirectory = Path.Combine(tempRoot, "Bin");
        Directory.CreateDirectory(binDirectory);
        File.WriteAllText(Path.Combine(binDirectory, "SilentHill.exe"), string.Empty);

        string path = ModDependencyAnalyzer.ResolveInstallDirectory(null, binDirectory);

        Assert.Equal(binDirectory, path);

        Directory.Delete(tempRoot, recursive: true);
    }

    [Fact]
    public void ResolveInstallDirectoryPrefersSelectedBinOverConfigDirectory()
    {
        string tempRoot = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N"));
        string selectedBin = Path.Combine(tempRoot, "Selected", "Bin");
        string configBin = Path.Combine(tempRoot, "Previous", "Bin");
        Directory.CreateDirectory(selectedBin);
        Directory.CreateDirectory(configBin);
        File.WriteAllText(Path.Combine(selectedBin, "SilentHill.exe"), string.Empty);
        File.WriteAllText(Path.Combine(configBin, "SilentHill.exe"), string.Empty);

        string path = ModDependencyAnalyzer.ResolveInstallDirectory(
            Path.Combine(configBin, "ShepherdPatch.ini"),
            selectedBin);

        Assert.Equal(selectedBin, path);
        Directory.Delete(tempRoot, recursive: true);
    }

    [Fact]
    public void GameBinValidationRequiresGameExecutable()
    {
        string tempRoot = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(tempRoot);
        File.WriteAllText(Path.Combine(tempRoot, "shv.dll"), string.Empty);

        Assert.False(ModDependencyAnalyzer.IsGameBinDirectory(tempRoot));

        Directory.Delete(tempRoot, recursive: true);
    }

    [Fact]
    public void ResolveInstallDirectoryDoesNotReturnMissingDefaultPath()
    {
        string path = ModDependencyAnalyzer.ResolveInstallDirectory(null);

        Assert.True(string.IsNullOrEmpty(path) || Directory.Exists(path));
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
