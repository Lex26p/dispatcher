using Dispatcher.Contracts.Events;

namespace Dispatcher.Contracts.Alarms;

public sealed record AlarmHistoryQueryResponseDto(
    IReadOnlyList<EventRecordDto> Items,
    int Page,
    int Limit,
    bool HasMore);
