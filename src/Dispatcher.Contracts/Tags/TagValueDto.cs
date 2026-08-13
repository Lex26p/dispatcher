namespace Dispatcher.Contracts.Tags;

public sealed record TagValueDto(
    string TagId,
    object? Value,
    DateTimeOffset Timestamp,
    bool Writable = false);
