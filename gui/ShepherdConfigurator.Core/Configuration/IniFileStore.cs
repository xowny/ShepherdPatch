using System;
using System.IO;

namespace ShepherdConfigurator.Configuration;

public static class IniFileStore
{
    public static bool TryLoad(string path, out IniDocument document)
    {
        try
        {
            document = IniDocument.Parse(File.ReadAllText(path));
            return true;
        }
        catch (Exception exception) when (exception is IOException or UnauthorizedAccessException)
        {
            document = IniDocument.Parse(string.Empty);
            return false;
        }
    }

    public static void Save(string path, IniDocument document)
    {
        File.WriteAllText(path, document.Serialize());
    }
}
