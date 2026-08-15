namespace Dispatcher.Contracts.Events;

public sealed record EventRecordDto(
    long EventId,
    DateTimeOffset Timestamp,
    EventCategoryDto Category,
    string Type,
    EventSeverityDto Severity,
    string Source,
    string Message,
    string? DataJson);
