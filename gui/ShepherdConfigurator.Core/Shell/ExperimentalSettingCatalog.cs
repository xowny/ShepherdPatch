using System;
using System.Collections.Frozen;
using System.Collections.Generic;
using System.Linq;

namespace ShepherdConfigurator.Shell;

public static class ExperimentalSettingCatalog
{
    private static readonly IReadOnlyDictionary<string, string[]> SettingsByCategory = new Dictionary<string, string[]>(StringComparer.OrdinalIgnoreCase)
    {
        ["Input"] =
        [
            "HardenDirectInputMouseDevice"
        ],
        ["Advanced"] =
        [
            "DisableLegacyPeriodicIconicTimer",
            "HardenLegacyGraphicsRecovery",
            "HardenLegacyThreadWrapper"
        ],
        ["Movies"] =
        [
            "ReduceMenuMovieStutter"
        ]
    };

    private static readonly FrozenSet<string> ExperimentalKeys = SettingsByCategory
        .Values
        .SelectMany(static values => values)
        .ToFrozenSet(StringComparer.OrdinalIgnoreCase);

    private static readonly FrozenDictionary<string, int> CountByCategory = SettingsByCategory
        .ToFrozenDictionary(static pair => pair.Key, static pair => pair.Value.Length, StringComparer.OrdinalIgnoreCase);

    public static bool IsExperimental(string key)
    {
        return ExperimentalKeys.Contains(key);
    }

    public static int CountForCategories(IEnumerable<string> categories)
    {
        HashSet<string> seen = new(StringComparer.OrdinalIgnoreCase);
        int count = 0;

        foreach (string category in categories)
        {
            if (!seen.Add(category))
            {
                continue;
            }

            if (CountByCategory.TryGetValue(category, out int categoryCount))
            {
                count += categoryCount;
            }
        }

        return count;
    }
}
