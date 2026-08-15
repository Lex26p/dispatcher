namespace Dispatcher.Server.Events;

public interface IEventJournalStore
{
    Task InitializeAsync(
        CancellationToken cancellationToken = default);

    Task AppendEventsAsync(
        IReadOnlyList<EventRecord> events,
        CancellationToken cancellationToken = default);

    Task<IReadOnlyList<EventRecord>> LoadAllEventsAsync(
        CancellationToken cancellationToken = default);
}
