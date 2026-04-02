using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Versioning;
using Microsoft.Win32;

namespace ShepherdConfigurator.Services;

[SupportedOSPlatform("windows")]
public static class RuntimePrerequisiteAnalyzer
{
    private static readonly RuntimePrerequisite[] KnownPrerequisites =
    [
        new(
            Id: "vcpp-x86",
            DisplayName: "Microsoft Visual C++ Redistributable (x86)",
            Description: "Required for builds that link the Microsoft C/C++ runtime dynamically.",
            DownloadUrl: "https://aka.ms/vc14/vc_redist.x86.exe",
            IsRequired: false,
            IsInstalled: static () => IsVcRedistInstalled("X86")),
        new(
            Id: "dotnet-runtime-8-x64",
            DisplayName: ".NET 8 Runtime (x64)",
            Description: "Required for framework-dependent configurator builds.",
            DownloadUrl: "https://dotnet.microsoft.com/en-us/download/dotnet/8.0/runtime",
            IsRequired: false,
            IsInstalled: static () => IsDotNetRuntimeInstalled("Microsoft.NETCore.App", 8)),
        new(
            Id: "windows-app-runtime",
            DisplayName: "Windows App Runtime",
            Description: "Required for non-self-contained WinUI 3 builds.",
            DownloadUrl: "https://learn.microsoft.com/en-us/windows/apps/windows-app-sdk/downloads",
            IsRequired: false,
            IsInstalled: static () => true)
    ];

    public static IReadOnlyList<RuntimePrerequisite> GetMissingPrerequisites()
    {
        return GetMissingPrerequisites(KnownPrerequisites);
    }

    internal static IReadOnlyList<RuntimePrerequisite> GetMissingPrerequisites(IEnumerable<RuntimePrerequisite> prerequisites)
    {
        List<RuntimePrerequisite> missing = [];

        foreach (RuntimePrerequisite prerequisite in prerequisites)
        {
            if (!prerequisite.IsRequired)
            {
                continue;
            }

            if (!prerequisite.IsInstalled())
            {
                missing.Add(prerequisite);
            }
        }

        return missing;
    }

    internal static bool IsVcRedistInstalled(string architecture)
    {
        using RegistryKey? key = Registry.LocalMachine.OpenSubKey($@"SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\{architecture}");
        if (key is null)
        {
            return false;
        }

        object? installed = key.GetValue("Installed");
        return installed is int value && value == 1;
    }

    internal static bool IsDotNetRuntimeInstalled(string runtimeName, int majorVersion)
    {
        using RegistryKey? key = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\dotnet\Setup\InstalledVersions\x64\sharedfx");
        using RegistryKey? runtimeKey = key?.OpenSubKey(runtimeName);
        if (runtimeKey is null)
        {
            return false;
        }

        return runtimeKey.GetSubKeyNames().Any(name =>
            Version.TryParse(name, out Version? version) &&
            version.Major >= majorVersion);
    }
}

public sealed record RuntimePrerequisite(
    string Id,
    string DisplayName,
    string Description,
    string DownloadUrl,
    bool IsRequired,
    Func<bool> IsInstalled);
