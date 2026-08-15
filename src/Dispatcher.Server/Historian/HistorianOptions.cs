using Microsoft.Extensions.Configuration;

namespace Dispatcher.Server.Historian;

public sealed record HistorianOptions(
    int BufferCapacity,
    int BatchSize)
{
    public static HistorianOptions Create(
        IConfiguration configuration)
    {
        ArgumentNullException.ThrowIfNull(
            configuration);

        var bufferCapacity =
            configuration.GetValue(
                "Historian:BufferCapacity",
                10000);

        var batchSize =
            configuration.GetValue(
                "Historian:BatchSize",
                256);

        if (bufferCapacity <= 0)
        {
            throw new InvalidOperationException(
                "Historian:BufferCapacity must be greater than zero.");
        }

        if (batchSize <= 0)
        {
            throw new InvalidOperationException(
                "Historian:BatchSize must be greater than zero.");
        }

        if (batchSize > bufferCapacity)
        {
            throw new InvalidOperationException(
                "Historian:BatchSize cannot exceed Historian:BufferCapacity.");
        }

        return new HistorianOptions(
            bufferCapacity,
            batchSize);
    }
}
