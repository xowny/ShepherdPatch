using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace ShepherdConfigurator.Configuration;

public sealed class IniDocument
{
    private readonly List<IniLine> _lines;
    private readonly Dictionary<string, int> _indexByKey;

    private IniDocument(List<IniLine> lines, Dictionary<string, int> indexByKey)
    {
        _lines = lines;
        _indexByKey = indexByKey;
    }

    public static IniDocument Parse(string text)
    {
        List<IniLine> lines = [];
        Dictionary<string, int> indexByKey = new(StringComparer.OrdinalIgnoreCase);

        using StringReader reader = new(text);
        while (reader.ReadLine() is { } rawLine)
        {
            IniLine line = IniLine.Parse(rawLine);
            lines.Add(line);

            if (line.Kind == IniLineKind.KeyValue && line.Key is not null && !indexByKey.ContainsKey(line.Key))
            {
                indexByKey[line.Key] = lines.Count - 1;
            }
        }

        if (text.Length > 0 && EndsWithLineBreak(text))
        {
            lines.Add(IniLine.Parse(string.Empty));
        }

        return new IniDocument(lines, indexByKey);
    }

    public string? GetValue(string key)
    {
        return _indexByKey.TryGetValue(key, out int index)
            ? _lines[index].Value
            : null;
    }

    public void SetValue(string key, string value)
    {
        if (_indexByKey.TryGetValue(key, out int existingIndex))
        {
            _lines[existingIndex].Value = value;
            return;
        }

        if (_lines.Count > 0 && !string.IsNullOrEmpty(_lines[^1].RawText))
        {
            _lines.Add(IniLine.Parse(string.Empty));
        }

        _lines.Add(new IniLine(IniLineKind.KeyValue, $"{key} = {value}", key, value));
        _indexByKey[key] = _lines.Count - 1;
    }

    public string Serialize()
    {
        StringBuilder builder = new();
        for (int index = 0; index < _lines.Count; index++)
        {
            if (index > 0)
            {
                builder.AppendLine();
            }

            builder.Append(_lines[index].ToOutputString());
        }

        return builder.ToString();
    }

    private static bool EndsWithLineBreak(string text)
    {
        char lastCharacter = text[^1];
        return lastCharacter is '\n' or '\r';
    }

    private enum IniLineKind
    {
        Raw,
        KeyValue,
    }

    private sealed class IniLine
    {
        public IniLineKind Kind { get; }
        public string RawText { get; }
        public string? Key { get; }
        public string? Value { get; set; }

        public IniLine(IniLineKind kind, string rawText, string? key = null, string? value = null)
        {
            Kind = kind;
            RawText = rawText;
            Key = key;
            Value = value;
        }

        public static IniLine Parse(string rawLine)
        {
            ReadOnlySpan<char> rawSpan = rawLine.AsSpan();
            int separatorIndex = rawSpan.IndexOf('=');
            if (separatorIndex <= 0)
            {
                return new IniLine(IniLineKind.Raw, rawLine);
            }

            string key = rawSpan[..separatorIndex].Trim().ToString();
            string value = rawSpan[(separatorIndex + 1)..].Trim().ToString();
            if (key.Length == 0)
            {
                return new IniLine(IniLineKind.Raw, rawLine);
            }

            return new IniLine(IniLineKind.KeyValue, rawLine, key, value);
        }

        public string ToOutputString()
        {
            return Kind == IniLineKind.KeyValue && Key is not null && Value is not null
                ? $"{Key} = {Value}"
                : RawText;
        }
    }
}
