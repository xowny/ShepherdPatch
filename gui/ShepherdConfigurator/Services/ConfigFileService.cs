using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using ShepherdConfigurator.Configuration;

namespace ShepherdConfigurator.Services;

public static class ConfigFileService
{
    public static string? ResolveConfigPath()
    {
        List<string> candidates = [];

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

    public static IniDocument Load(string path)
    {
        return IniDocument.Parse(File.ReadAllText(path));
    }

    public static void Save(string path, IniDocument document)
    {
        File.WriteAllText(path, document.Serialize());
    }
}
