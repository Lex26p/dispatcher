namespace Dispatcher.Contracts.Historian;

public sealed record HistorySeriesDto(
    string TagId,
    bool Truncated,
    IReadOnlyList<HistorySampleDto> Samples);
