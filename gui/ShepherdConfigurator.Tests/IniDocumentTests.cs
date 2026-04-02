using ShepherdConfigurator.Configuration;

namespace ShepherdConfigurator.Tests;

public sealed class IniDocumentTests
{
    [Fact]
    public void ParseReadsExistingValues()
    {
        const string text = """
            # comment
            EnableRawMouseInput = true
            TargetFrameRate = 60
            """;

        IniDocument document = IniDocument.Parse(text);

        Assert.Equal("true", document.GetValue("EnableRawMouseInput"));
        Assert.Equal("60", document.GetValue("TargetFrameRate"));
        Assert.Null(document.GetValue("MissingKey"));
    }

    [Fact]
    public void SetValueUpdatesExistingLineWithoutDroppingOtherContent()
    {
        const string text = """
            # comment
            EnableRawMouseInput = true
            TargetFrameRate = 60
            """;

        IniDocument document = IniDocument.Parse(text);
        document.SetValue("TargetFrameRate", "120");

        string serialized = document.Serialize();

        Assert.Contains("# comment", serialized);
        Assert.Contains("EnableRawMouseInput = true", serialized);
        Assert.Contains("TargetFrameRate = 120", serialized);
        Assert.DoesNotContain("TargetFrameRate = 60", serialized);
    }

    [Fact]
    public void SetValueAppendsMissingLine()
    {
        const string text = """
            EnableRawMouseInput = true
            """;

        IniDocument document = IniDocument.Parse(text);
        document.SetValue("InvertRawMouseY", "false");

        string serialized = document.Serialize();

        Assert.Contains("EnableRawMouseInput = true", serialized);
        Assert.Contains("InvertRawMouseY = false", serialized);
    }
}
