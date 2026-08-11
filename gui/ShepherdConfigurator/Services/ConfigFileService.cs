using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using ShepherdConfigurator.Configuration;

namespace ShepherdConfigurator.Services;

public static class ConfigFileService
{
    private static readonly string SelectedGameBinPath = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "ShepherdPatch",
        "game-bin.txt");

    private static readonly string LaunchThroughSteamPath = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "ShepherdPatch",
        "launch-through-steam.txt");

    public static string? ResolveConfigPath()
    {
        List<string> candidates = [];

        string? selectedGameBin = ResolveSelectedGameBinDirectory();
        if (selectedGameBin is not null)
        {
            candidates.Add(Path.Combine(selectedGameBin, "ShepherdPatch.ini"));
        }

        string baseDirectory = AppContext.BaseDirectory;
        candidates.Add(Path.Combine(baseDirectory, "ShepherdPatch.ini"));

        DirectoryInfo? current = new(baseDirectory);
        for (int index = 0; index < 6 && current is not null; index++)
        {
            candidates.Add(Path.Combine(current.FullName, "ShepherdPatch.ini"));
            current = current.Parent;
        }

        candidates.Add(Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86),
            "Steam", "steamapps", "common", "Silent Hill Homecoming", "Bin", "ShepherdPatch.ini"));

        return candidates.FirstOrDefault(File.Exists);
    }

    public static string? ResolveSelectedGameBinDirectory()
    {
        try
        {
            if (!File.Exists(SelectedGameBinPath))
            {
                return null;
            }

            string path = File.ReadAllText(SelectedGameBinPath).Trim();
            return IsGameBinDirectory(path) ? Path.GetFullPath(path) : null;
        }
        catch (IOException)
        {
            return null;
        }
        catch (UnauthorizedAccessException)
        {
            return null;
        }
    }

    public static bool SetSelectedGameBinDirectory(string path)
    {
        if (!IsGameBinDirectory(path))
        {
            return false;
        }

        string? parentDirectory = Path.GetDirectoryName(SelectedGameBinPath);
        if (string.IsNullOrWhiteSpace(parentDirectory))
        {
            return false;
        }

        try
        {
            Directory.CreateDirectory(parentDirectory);
            File.WriteAllText(SelectedGameBinPath, Path.GetFullPath(path));
            return true;
        }
        catch (IOException)
        {
            return false;
        }
        catch (UnauthorizedAccessException)
        {
            return false;
        }
    }

    public static bool ResolveLaunchThroughSteamPreference()
    {
        try
        {
            return File.Exists(LaunchThroughSteamPath) &&
                   bool.TryParse(File.ReadAllText(LaunchThroughSteamPath).Trim(), out bool value) &&
                   value;
        }
        catch (IOException)
        {
            return false;
        }
        catch (UnauthorizedAccessException)
        {
            return false;
        }
    }

    public static void SetLaunchThroughSteamPreference(bool enabled)
    {
        string? parentDirectory = Path.GetDirectoryName(LaunchThroughSteamPath);
        if (string.IsNullOrWhiteSpace(parentDirectory))
        {
            return;
        }

        try
        {
            Directory.CreateDirectory(parentDirectory);
            File.WriteAllText(LaunchThroughSteamPath, enabled.ToString());
        }
        catch (IOException)
        {
        }
        catch (UnauthorizedAccessException)
        {
        }
    }

    public static bool IsGameBinDirectory(string? path)
    {
        return !string.IsNullOrWhiteSpace(path) &&
               Directory.Exists(path) &&
               (File.Exists(Path.Combine(path, "SilentHill.exe")) ||
                File.Exists(Path.Combine(path, "shv.dll")));
    }

    public static IniDocument Load(string path)
    {
        return IniDocument.Parse(File.ReadAllText(path));
    }

    public static void Save(string path, IniDocument document)
    {
        File.WriteAllText(path, document.Serialize());
    }
}
