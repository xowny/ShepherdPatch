using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace ShepherdConfigurator.Services;

public static class ModDependencyAnalyzer
{
    private static readonly string[] RequiredFiles =
    [
        "ShepherdPatch.asi",
        "version.dll"
    ];

    public static DependencyStatus GetStatus(
        string? configPath,
        string? baseDirectory = null,
        string? selectedGameBinDirectory = null)
    {
        string installDirectory = ResolveInstallDirectory(configPath, selectedGameBinDirectory);

        List<string> missingFiles = string.IsNullOrWhiteSpace(installDirectory)
            ? [.. RequiredFiles]
            : RequiredFiles
                .Where(fileName => !File.Exists(Path.Combine(installDirectory, fileName)))
                .ToList();

        return new DependencyStatus(
            installDirectory,
            missingFiles,
            FindBundledDependencySource(baseDirectory ?? AppContext.BaseDirectory));
    }

    public static string ResolveInstallDirectory(
        string? configPath,
        string? selectedGameBinDirectory = null)
    {
        if (!string.IsNullOrWhiteSpace(selectedGameBinDirectory) &&
            IsGameBinDirectory(selectedGameBinDirectory))
        {
            return Path.GetFullPath(selectedGameBinDirectory);
        }

        if (!string.IsNullOrWhiteSpace(configPath))
        {
            string? directory = Path.GetDirectoryName(configPath);
            if (!string.IsNullOrWhiteSpace(directory) && IsGameBinDirectory(directory))
            {
                return Path.GetFullPath(directory);
            }
        }

        string defaultSteamBin = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86),
            "Steam", "steamapps", "common", "Silent Hill Homecoming", "Bin");

        return IsGameBinDirectory(defaultSteamBin) ? defaultSteamBin : string.Empty;
    }

    public static string? FindBundledDependencySource(string baseDirectory)
    {
        List<string> candidates = [];

        candidates.Add(baseDirectory);

        DirectoryInfo? current = new(baseDirectory);
        for (int index = 0; index < 8 && current is not null; index++)
        {
            candidates.Add(Path.Combine(current.FullName, "build", "Release"));
            candidates.Add(current.FullName);
            current = current.Parent;
        }

        foreach (string candidate in candidates.Distinct(StringComparer.OrdinalIgnoreCase))
        {
            if (RequiredFiles.All(fileName => File.Exists(Path.Combine(candidate, fileName))))
            {
                return candidate;
            }
        }

        return null;
    }

    public static bool IsGameBinDirectory(string? directory)
    {
        return !string.IsNullOrWhiteSpace(directory) &&
               Directory.Exists(directory) &&
               File.Exists(Path.Combine(directory, "SilentHill.exe"));
    }
}

public sealed record DependencyStatus(
    string InstallDirectory,
    IReadOnlyList<string> MissingFiles,
    string? BundledSourceDirectory)
{
    public bool HasMissingDependencies => MissingFiles.Count > 0;
}
