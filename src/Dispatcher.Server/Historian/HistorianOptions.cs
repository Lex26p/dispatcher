using Microsoft.Extensions.Configuration;

namespace Dispatcher.Server.Historian;

public sealed record HistorianOptions(
    int BufferCapacity,
    int BatchSize,
    int PeriodicScanMilliseconds = 100,
    int RetentionCleanupIntervalMinutes = 60)
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

        var periodicScanMilliseconds =
            configuration.GetValue(
                "Historian:PeriodicScanMilliseconds",
                100);

        var retentionCleanupIntervalMinutes =
            configuration.GetValue(
                "Historian:RetentionCleanupIntervalMinutes",
                60);

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

        if (periodicScanMilliseconds is < 10
            or > HistorianPolicyValidator.MinPeriodicIntervalMilliseconds)
        {
            throw new InvalidOperationException(
                $"Historian:PeriodicScanMilliseconds must be between 10 and " +
                $"{HistorianPolicyValidator.MinPeriodicIntervalMilliseconds}.");
        }

        if (retentionCleanupIntervalMinutes <= 0)
        {
            throw new InvalidOperationException(
                "Historian:RetentionCleanupIntervalMinutes must be greater than zero.");
        }

        return new HistorianOptions(
            bufferCapacity,
            batchSize,
            periodicScanMilliseconds,
            retentionCleanupIntervalMinutes);
    }
}
