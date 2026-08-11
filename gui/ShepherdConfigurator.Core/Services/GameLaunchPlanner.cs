namespace ShepherdConfigurator.Services;

public enum GameLaunchKind
{
    Steam,
    Executable
}

public sealed record GameLaunchPlan(
    GameLaunchKind Kind,
    string FileName,
    string? WorkingDirectory);

public static class GameLaunchPlanner
{
    public const string SteamAppId = "19000";

    public static GameLaunchPlan? Create(string installDirectory, bool launchThroughSteam = false)
    {
        if (string.IsNullOrWhiteSpace(installDirectory))
        {
            return null;
        }

        string fullInstallDirectory;
        try
        {
            fullInstallDirectory = Path.GetFullPath(installDirectory);
        }
        catch (Exception exception) when (exception is ArgumentException or NotSupportedException
                                          or PathTooLongException)
        {
            return null;
        }

        string executablePath = Path.Combine(fullInstallDirectory, "SilentHill.exe");
        if (!File.Exists(executablePath))
        {
            return null;
        }

        if (launchThroughSteam)
        {
            if (!IsSteamInstallation(fullInstallDirectory))
            {
                return null;
            }

            return new GameLaunchPlan(
                GameLaunchKind.Steam,
                $"steam://run/{SteamAppId}",
                WorkingDirectory: null);
        }

        return new GameLaunchPlan(
            GameLaunchKind.Executable,
            executablePath,
            fullInstallDirectory);
    }

    private static bool IsSteamInstallation(string installDirectory)
    {
        DirectoryInfo binDirectory = new(installDirectory);
        DirectoryInfo? gameDirectory = binDirectory.Parent;
        DirectoryInfo? commonDirectory = gameDirectory?.Parent;
        DirectoryInfo? steamAppsDirectory = commonDirectory?.Parent;

        if (!binDirectory.Name.Equals("Bin", StringComparison.OrdinalIgnoreCase) ||
            commonDirectory is null ||
            !commonDirectory.Name.Equals("common", StringComparison.OrdinalIgnoreCase) ||
            steamAppsDirectory is null)
        {
            return false;
        }

        string manifestPath = Path.Combine(
            steamAppsDirectory.FullName,
            $"appmanifest_{SteamAppId}.acf");
        return File.Exists(manifestPath);
    }
}
