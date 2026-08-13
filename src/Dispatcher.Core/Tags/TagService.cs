using System.Collections.Concurrent;

namespace Dispatcher.Core.Tags;

public sealed class TagService
{
    private readonly ConcurrentDictionary<string, TagValue> _values =
        new(StringComparer.Ordinal);

    public TagValue Set(string tagId, object? value)
    {
        return Set(tagId, value, DateTimeOffset.UtcNow);
    }

    public TagValue Set(string tagId, object? value, DateTimeOffset timestamp)
    {
        ValidateTagId(tagId);

        var tagValue = new TagValue(tagId, value, timestamp);
        _values[tagId] = tagValue;

        return tagValue;
    }

    public TagValue? Get(string tagId)
    {
        ValidateTagId(tagId);

        return _values.TryGetValue(tagId, out var value)
            ? value
            : null;
    }

    public IReadOnlyList<TagValue> GetAll()
    {
        return _values.Values
            .OrderBy(tag => tag.TagId, StringComparer.Ordinal)
            .ToArray();
    }

    private static void ValidateTagId(string tagId)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(tagId);
    }
}
