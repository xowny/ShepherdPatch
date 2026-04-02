using System;
using System.Collections.Generic;
namespace ShepherdConfigurator.Shell;

public static class CardVisibilityFilter
{
    public static bool ShouldShowCard(string selectedCategory, string query, string cardCategory, IReadOnlyList<string> tokens)
    {
        bool categoryMatch = string.Equals(selectedCategory, "All", StringComparison.OrdinalIgnoreCase) ||
                             string.Equals(selectedCategory, cardCategory, StringComparison.OrdinalIgnoreCase);
        if (!categoryMatch)
        {
            return false;
        }

        ReadOnlySpan<char> normalizedQuery = query.AsSpan().Trim();
        if (normalizedQuery.Length == 0)
        {
            return true;
        }

        if (cardCategory.AsSpan().Contains(normalizedQuery, StringComparison.OrdinalIgnoreCase))
        {
            return true;
        }

        foreach (string token in tokens)
        {
            if (token.AsSpan().Contains(normalizedQuery, StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }
        }

        return false;
    }

    public static IReadOnlyList<T> FilterVisible<T>(IEnumerable<T> items, string selectedCategory, string query, Func<T, string> categorySelector, Func<T, IReadOnlyList<string>> tokensSelector)
    {
        List<T> visibleItems = [];

        foreach (T item in items)
        {
            if (ShouldShowCard(selectedCategory, query, categorySelector(item), tokensSelector(item)))
            {
                visibleItems.Add(item);
            }
        }

        return visibleItems;
    }
}
