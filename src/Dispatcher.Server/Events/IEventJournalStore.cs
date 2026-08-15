namespace Dispatcher.Server.Events;

public interface IEventJournalStore
{
    Task InitializeAsync(
        CancellationToken cancellationToken = default);

    Task<IReadOnlyList<EventRecord>> AppendEventsAsync(
        IReadOnlyList<EventRecord> events,
        CancellationToken cancellationToken = default);

    Task<IReadOnlyList<EventRecord>> QueryEventsAsync(
        DateTimeOffset from,
        DateTimeOffset to,
        EventCategory? category,
        EventSeverity? severity,
        string? source,
        string? text,
        int offset,
        int limit,
        CancellationToken cancellationToken = default);

    Task<IReadOnlyList<EventRecord>> LoadAllEventsAsync(
        CancellationToken cancellationToken = default);
}
