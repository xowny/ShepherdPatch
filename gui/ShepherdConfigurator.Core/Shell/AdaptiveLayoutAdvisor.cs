using System;
using System.Collections.Generic;
namespace ShepherdConfigurator.Shell;

public enum AdaptiveLayoutMode
{
    Compact,
    Standard,
    Wide
}

public static class AdaptiveLayoutAdvisor
{
    public static AdaptiveLayoutMode GetMode(double width)
    {
        if (width < 1180)
        {
            return AdaptiveLayoutMode.Compact;
        }

        if (width < 1540)
        {
            return AdaptiveLayoutMode.Standard;
        }

        return AdaptiveLayoutMode.Wide;
    }

    public static int GetColumnCount(double width)
    {
        return GetColumnCount(GetMode(width));
    }

    public static int GetColumnCount(AdaptiveLayoutMode mode)
    {
        return mode == AdaptiveLayoutMode.Compact ? 1 : 2;
    }

    public static IReadOnlyList<IReadOnlyList<T>> DistributeCards<T>(IReadOnlyList<T> items, Func<T, int> weightSelector, int columnCount)
    {
        if (columnCount <= 1 || items.Count <= 1)
        {
            return [items.ToArray()];
        }

        List<(T Item, int Weight, int Index)> ordered = items
            .Select((item, index) => (Item: item, Weight: Math.Max(1, weightSelector(item)), Index: index))
            .OrderByDescending(entry => entry.Weight)
            .ThenBy(entry => entry.Index)
            .ToList();

        List<List<(T Item, int Index)>> columns = [];
        int[] columnWeights = new int[columnCount];

        for (int columnIndex = 0; columnIndex < columnCount; columnIndex++)
        {
            columns.Add([]);
        }

        foreach ((T item, int weight, int index) in ordered)
        {
            int targetColumn = 0;
            for (int columnIndex = 1; columnIndex < columnCount; columnIndex++)
            {
                if (columnWeights[columnIndex] < columnWeights[targetColumn])
                {
                    targetColumn = columnIndex;
                }
            }

            columns[targetColumn].Add((item, index));
            columnWeights[targetColumn] += weight;
        }

        return columns
            .Select(column => (IReadOnlyList<T>)column
                .OrderBy(entry => entry.Index)
                .Select(entry => entry.Item)
                .ToArray())
            .ToArray();
    }

    public static IReadOnlyList<IReadOnlyList<T>> DistributeCards<T>(IReadOnlyList<T> items, Func<T, int> weightSelector, AdaptiveLayoutMode mode)
    {
        return DistributeCards(items, weightSelector, GetColumnCount(mode));
    }
}
