namespace Dispatcher.Contracts.Events;

public sealed record EventQueryResponseDto(
    int Page,
    int Limit,
    bool HasMore,
    IReadOnlyList<EventRecordDto> Items);
