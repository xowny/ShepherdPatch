using ShepherdConfigurator.Shell;

namespace ShepherdConfigurator.Tests;

public sealed class CardVisibilityFilterTests
{
    [Fact]
    public void ShowsCardWhenCategoryMatchesAndQueryIsEmpty()
    {
        bool visible = CardVisibilityFilter.ShouldShowCard(
            selectedCategory: "Display",
            query: "",
            cardCategory: "Display",
            tokens: ["display", "resolution", "borderless"]);

        Assert.True(visible);
    }

    [Fact]
    public void HidesCardWhenCategoryDoesNotMatch()
    {
        bool visible = CardVisibilityFilter.ShouldShowCard(
            selectedCategory: "Input",
            query: "",
            cardCategory: "Display",
            tokens: ["display", "resolution", "borderless"]);

        Assert.False(visible);
    }

    [Fact]
    public void MatchesSearchAgainstTokensAndCategory()
    {
        Assert.True(CardVisibilityFilter.ShouldShowCard(
            selectedCategory: "All",
            query: "border",
            cardCategory: "Display",
            tokens: ["display", "resolution", "borderless"]));

        Assert.True(CardVisibilityFilter.ShouldShowCard(
            selectedCategory: "All",
            query: "movie",
            cardCategory: "Movies",
            tokens: ["intro", "bink"]));

        Assert.False(CardVisibilityFilter.ShouldShowCard(
            selectedCategory: "All",
            query: "thread",
            cardCategory: "Display",
            tokens: ["display", "resolution", "borderless"]));
    }

    [Fact]
    public void FilterVisibleReturnsOnlyMatchingCards()
    {
        TestCard[] cards =
        [
            new("Display", ["display", "resolution", "borderless"]),
            new("Movies", ["intro", "bink"]),
            new("Advanced", ["legacy", "thread"])
        ];

        IReadOnlyList<TestCard> visible = CardVisibilityFilter.FilterVisible(
            cards,
            selectedCategory: "All",
            query: "movie",
            static card => card.Category,
            static card => card.Tokens);

        Assert.Single(visible);
        Assert.Equal("Movies", visible[0].Category);
    }

    private sealed record TestCard(string Category, string[] Tokens);
}
