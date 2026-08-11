using System;
using System.IO;

namespace ShepherdConfigurator.Configuration;

public enum IniFileLoadStatus
{
    Success,
    Missing,
    Unreadable
}

public static class IniFileStore
{
    public static IniFileLoadStatus Load(string path, out IniDocument document)
    {
        try
        {
            document = IniDocument.Parse(File.ReadAllText(path));
            return IniFileLoadStatus.Success;
        }
        catch (Exception exception) when (exception is FileNotFoundException or DirectoryNotFoundException)
        {
            document = IniDocument.Parse(string.Empty);
            return IniFileLoadStatus.Missing;
        }
        catch (Exception exception) when (exception is IOException or UnauthorizedAccessException)
        {
            document = IniDocument.Parse(string.Empty);
            return IniFileLoadStatus.Unreadable;
        }
    }

    public static void Save(string path, IniDocument document)
    {
        File.WriteAllText(path, document.Serialize());
    }
}
