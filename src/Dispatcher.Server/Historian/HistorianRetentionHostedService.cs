namespace Dispatcher.Server.Historian;

public sealed class HistorianRetentionHostedService : BackgroundService
{
    private readonly IHistorySampleStore _store;
    private readonly HistorianPolicyCatalog _policies;
    private readonly HistorianOptions _options;
    private readonly ILogger<HistorianRetentionHostedService> _logger;

    private long _deletedSampleCount;
    private long _cleanupRunCount;

    public HistorianRetentionHostedService(
        IHistorySampleStore store,
        HistorianPolicyCatalog policies,
        HistorianOptions options,
        ILogger<HistorianRetentionHostedService> logger)
    {
        _store =
            store;
        _policies =
            policies;
        _options =
            options;
        _logger =
            logger;
    }

    public long DeletedSampleCount =>
        Interlocked.Read(
            ref _deletedSampleCount);

    public long CleanupRunCount =>
        Interlocked.Read(
            ref _cleanupRunCount);

    public async Task<int> CleanupOnceAsync(
        DateTimeOffset now,
        CancellationToken cancellationToken = default)
    {
        var totalDeleted =
            0;

        foreach (var policy in _policies.Policies)
        {
            var cutoff =
                now.ToUniversalTime()
                    .AddDays(
                        -policy.RetentionDays);

            totalDeleted +=
                await _store.DeleteBeforeAsync(
                    policy.TagId,
                    cutoff,
                    cancellationToken);
        }

        Interlocked.Increment(
            ref _cleanupRunCount);

        if (totalDeleted > 0)
        {
            Interlocked.Add(
                ref _deletedSampleCount,
                totalDeleted);

            _logger.LogInformation(
                "Historian retention cleanup deleted {DeletedSampleCount} sample(s).",
                totalDeleted);
        }

        return totalDeleted;
    }

    protected override async Task ExecuteAsync(
        CancellationToken stoppingToken)
    {
        var delay =
            TimeSpan.FromMinutes(
                _options.RetentionCleanupIntervalMinutes);

        while (!stoppingToken.IsCancellationRequested)
        {
            try
            {
                await CleanupOnceAsync(
                    DateTimeOffset.UtcNow,
                    stoppingToken);
            }
            catch (OperationCanceledException)
                when (stoppingToken.IsCancellationRequested)
            {
                break;
            }
            catch (Exception exception)
            {
                _logger.LogError(
                    exception,
                    "Historian retention cleanup failed.");
            }

            try
            {
                await Task.Delay(
                    delay,
                    stoppingToken);
            }
            catch (OperationCanceledException)
                when (stoppingToken.IsCancellationRequested)
            {
                break;
            }
        }
    }
}
