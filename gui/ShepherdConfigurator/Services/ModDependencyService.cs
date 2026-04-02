using System.IO;
using System.Diagnostics;

namespace ShepherdConfigurator.Services;

public static class ModDependencyService
{
    public static DependencyStatus GetStatus()
    {
        return ModDependencyAnalyzer.GetStatus(ConfigFileService.ResolveConfigPath());
    }

    public static void OpenDirectory(string path)
    {
        Directory.CreateDirectory(path);
        Process.Start(new ProcessStartInfo
        {
            FileName = path,
            UseShellExecute = true
        });
    }
}
