using System;
using System.IO;
using System.Diagnostics;

namespace ShepherdConfigurator.Services;

public static class ModDependencyService
{
    public static DependencyStatus GetStatus()
    {
        return ModDependencyAnalyzer.GetStatus(
            ConfigFileService.ResolveConfigPath(),
            selectedGameBinDirectory: ConfigFileService.ResolveSelectedGameBinDirectory());
    }

    public static bool OpenDirectory(string path)
    {
        if (string.IsNullOrWhiteSpace(path) || !Directory.Exists(path))
        {
            return false;
        }

        try
        {
            Process.Start(new ProcessStartInfo
            {
                FileName = path,
                UseShellExecute = true
            });
            return true;
        }
        catch (System.ComponentModel.Win32Exception)
        {
            return false;
        }
    }

    public static bool LaunchGame(string installDirectory, bool launchThroughSteam)
    {
        GameLaunchPlan? launchPlan = GameLaunchPlanner.Create(
            installDirectory,
            launchThroughSteam);
        return launchPlan is not null && LaunchGame(launchPlan);
    }

    public static bool LaunchGame(GameLaunchPlan launchPlan)
    {
        try
        {
            Process.Start(new ProcessStartInfo
            {
                FileName = launchPlan.FileName,
                WorkingDirectory = launchPlan.WorkingDirectory ?? string.Empty,
                UseShellExecute = true
            });
            return true;
        }
        catch (Exception exception) when (exception is System.ComponentModel.Win32Exception
                                          or InvalidOperationException)
        {
            return false;
        }
    }
}
