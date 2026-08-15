using Microsoft.Extensions.Configuration;

namespace Dispatcher.Server.Events;

public sealed record EventJournalOptions(
    int BufferCapacity,
    int BatchSize)
{
    public static EventJournalOptions Create(
        IConfiguration configuration)
    {
        ArgumentNullException.ThrowIfNull(
            configuration);

        var bufferCapacity =
            configuration.GetValue(
                "EventJournal:BufferCapacity",
                4096);

        var batchSize =
            configuration.GetValue(
                "EventJournal:BatchSize",
                128);

        if (bufferCapacity <= 0)
        {
            throw new InvalidOperationException(
                "EventJournal:BufferCapacity must be greater than zero.");
        }

        if (batchSize <= 0)
        {
            throw new InvalidOperationException(
                "EventJournal:BatchSize must be greater than zero.");
        }

        if (batchSize > bufferCapacity)
        {
            throw new InvalidOperationException(
                "EventJournal:BatchSize cannot exceed EventJournal:BufferCapacity.");
        }

        return new EventJournalOptions(
            bufferCapacity,
            batchSize);
    }
}
