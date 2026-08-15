namespace Dispatcher.Server.Historian;

public interface IHistorySampleStore
{
    Task InitializeAsync(
        CancellationToken cancellationToken = default);

    Task AppendAsync(
        IReadOnlyList<HistorySample> samples,
        CancellationToken cancellationToken = default);

    Task<int> DeleteBeforeAsync(
        string tagId,
        DateTimeOffset cutoff,
        CancellationToken cancellationToken = default);
}
