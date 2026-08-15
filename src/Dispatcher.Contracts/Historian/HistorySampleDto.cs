namespace Dispatcher.Contracts.Historian;

public sealed record HistorySampleDto(
    DateTimeOffset Timestamp,
    HistoryValueTypeDto ValueType,
    string? ValueText);
