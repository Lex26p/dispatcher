namespace Dispatcher.Server.Events;

public sealed record EventRecord(
    long EventId,
    DateTimeOffset Timestamp,
    EventCategory Category,
    string Type,
    EventSeverity Severity,
    string Source,
    string Message,
    string? DataJson);
