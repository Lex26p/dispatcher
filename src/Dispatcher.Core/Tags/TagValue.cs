namespace Dispatcher.Core.Tags;

public sealed record TagValue(
    string TagId,
    object? Value,
    DateTimeOffset Timestamp);
