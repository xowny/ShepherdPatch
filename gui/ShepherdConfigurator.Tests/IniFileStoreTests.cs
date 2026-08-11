using ShepherdConfigurator.Configuration;

namespace ShepherdConfigurator.Tests;

public sealed class IniFileStoreTests : IDisposable
{
    private readonly string _directory = Path.Combine(
        Path.GetTempPath(), $"ShepherdConfiguratorTests-{Guid.NewGuid():N}");

    [Fact]
    public void LoadReportsMissingFile()
    {
        IniFileLoadStatus status = IniFileStore.Load(
            Path.Combine(_directory, "missing.ini"), out IniDocument document);

        Assert.Equal(IniFileLoadStatus.Missing, status);
        Assert.Equal(string.Empty, document.Serialize());
    }

    [Fact]
    public void LoadReportsLockedFileAsUnreadable()
    {
        Directory.CreateDirectory(_directory);
        string path = Path.Combine(_directory, "ShepherdPatch.ini");
        File.WriteAllText(path, "TargetFrameRate = 60");

        using FileStream lockStream = new(
            path, FileMode.Open, FileAccess.ReadWrite, FileShare.None);
        IniFileLoadStatus status = IniFileStore.Load(path, out IniDocument document);

        Assert.Equal(IniFileLoadStatus.Unreadable, status);
        Assert.Equal(string.Empty, document.Serialize());
    }

    [Fact]
    public void SaveCreatesAConfigurationFile()
    {
        Directory.CreateDirectory(_directory);
        string path = Path.Combine(_directory, "ShepherdPatch.ini");
        IniDocument document = IniDocument.Parse("TargetFrameRate = 60");

        IniFileStore.Save(path, document);

        Assert.Equal("TargetFrameRate = 60", File.ReadAllText(path));
    }

    public void Dispose()
    {
        if (Directory.Exists(_directory))
        {
            Directory.Delete(_directory, recursive: true);
        }
    }
}
