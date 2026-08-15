namespace Dispatcher.Server.Historian;

public sealed record HistorySample(
    long SampleId,
    string TagId,
    DateTimeOffset Timestamp,
    HistoryValueType ValueType,
    string? ValueText);
