using ShepherdConfigurator.Shell;

namespace ShepherdConfigurator.Tests;

public sealed class AdaptiveLayoutAdvisorTests
{
    [Theory]
    [InlineData(640, AdaptiveLayoutMode.Compact)]
    [InlineData(960, AdaptiveLayoutMode.Standard)]
    [InlineData(1680, AdaptiveLayoutMode.Wide)]
    public void ReturnsExpectedLayoutMode(double width, AdaptiveLayoutMode expected)
    {
        Assert.Equal(expected, AdaptiveLayoutAdvisor.GetMode(width));
    }

    [Theory]
    [InlineData(AdaptiveLayoutMode.Compact, 1)]
    [InlineData(AdaptiveLayoutMode.Standard, 1)]
    [InlineData(AdaptiveLayoutMode.Wide, 2)]
    public void ReturnsExpectedColumnCount(AdaptiveLayoutMode mode, int expected)
    {
        Assert.Equal(expected, AdaptiveLayoutAdvisor.GetColumnCount(mode));
    }

    [Fact]
    public void ValueGridWrapsItemsAcrossDeclaredColumns()
    {
        Assert.Equal(2, AdaptiveLayoutAdvisor.GetGridRowCount(itemCount: 4, columnCount: 2));
        Assert.Equal(new AdaptiveLayoutAdvisor.GridPosition(0, 0), AdaptiveLayoutAdvisor.GetGridPosition(0, 2));
        Assert.Equal(new AdaptiveLayoutAdvisor.GridPosition(0, 1), AdaptiveLayoutAdvisor.GetGridPosition(1, 2));
        Assert.Equal(new AdaptiveLayoutAdvisor.GridPosition(1, 0), AdaptiveLayoutAdvisor.GetGridPosition(2, 2));
        Assert.Equal(new AdaptiveLayoutAdvisor.GridPosition(1, 1), AdaptiveLayoutAdvisor.GetGridPosition(3, 2));
    }

    [Fact]
    public void DistributeCardsBalancesColumnsWithoutBreakingOrder()
    {
        string[] cards = ["Display", "Input", "FrameRate", "Stability", "Advanced", "Movies"];
        Dictionary<string, int> weights = new()
        {
            ["Display"] = 8,
            ["Input"] = 4,
            ["FrameRate"] = 5,
            ["Stability"] = 6,
            ["Advanced"] = 5,
            ["Movies"] = 3
        };

        IReadOnlyList<IReadOnlyList<string>> columns = AdaptiveLayoutAdvisor.DistributeCards(cards, card => weights[card], AdaptiveLayoutMode.Wide);

        Assert.Equal(2, columns.Count);
        Assert.Equal(["Display", "Advanced", "Movies"], columns[0]);
        Assert.Equal(["Input", "FrameRate", "Stability"], columns[1]);

        Assert.True(IsInSourceOrder(columns[0], cards));
        Assert.True(IsInSourceOrder(columns[1], cards));

        int leftWeight = columns[0].Sum(card => weights[card]);
        int rightWeight = columns[1].Sum(card => weights[card]);
        Assert.InRange(Math.Abs(leftWeight - rightWeight), 0, 1);
    }

    private static bool IsInSourceOrder(IEnumerable<string> column, IReadOnlyList<string> source)
    {
        int lastIndex = -1;
        foreach (string card in column)
        {
            int index = -1;
            for (int i = 0; i < source.Count; i++)
            {
                if (source[i] == card)
                {
                    index = i;
                    break;
                }
            }

            if (index <= lastIndex)
            {
                return false;
            }

            lastIndex = index;
        }

        return true;
    }
}
