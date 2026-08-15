namespace Dispatcher.Contracts.Historian;

public sealed record HistoryQueryResponseDto(
    DateTimeOffset From,
    DateTimeOffset To,
    HistoryQueryOrderDto Order,
    int Limit,
    IReadOnlyList<HistorySeriesDto> Series);
