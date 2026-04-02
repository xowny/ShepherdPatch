using System.Text;

namespace ShepherdConfigurator.Shell;

public static class ConfiguratorStatusSummary
{
    public static string Build(
        int visibleSectionCount,
        string selectedCategory,
        string query,
        bool isDirty,
        int visibleExperimentalCount)
    {
        string scope = string.Equals(selectedCategory, "All", System.StringComparison.OrdinalIgnoreCase)
            ? $"{visibleSectionCount} sections visible"
            : $"{selectedCategory} view";

        string search = string.IsNullOrWhiteSpace(query)
            ? "no filter"
            : $"filter: \"{query.Trim()}\"";

        string changes = isDirty ? "unsaved changes" : "saved state";
        StringBuilder builder = new(capacity: 128);

        builder.Append(scope);
        builder.Append(" • ");
        builder.Append(search);
        builder.Append(" • ");
        builder.Append(changes);

        if (visibleExperimentalCount <= 0)
        {
            return builder.ToString();
        }

        string experimental = visibleExperimentalCount == 1
            ? "1 experimental option in view"
            : $"{visibleExperimentalCount} experimental options in view";

        builder.Append(" • ");
        builder.Append(experimental);
        return builder.ToString();
    }
}
