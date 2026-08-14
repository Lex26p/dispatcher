using Dispatcher.Server.Configuration;
using Dispatcher.Snmp;
using Dispatcher.Snmp.Configuration;

namespace Dispatcher.Server.Runtime;

public sealed class SnmpRuntimeHostedService : IHostedService
{
    private readonly ConfigurationCatalog _configuration;
    private readonly SnmpPollingService _pollingService;
    private readonly IHostApplicationLifetime _applicationLifetime;
    private readonly ILogger<SnmpRuntimeHostedService> _logger;
    private readonly SemaphoreSlim _stateLock = new(1, 1);
    private readonly List<RunningPollingLoop> _running = [];

    public SnmpRuntimeHostedService(
        ConfigurationCatalog configuration,
        SnmpPollingService pollingService,
        IHostApplicationLifetime applicationLifetime,
        ILogger<SnmpRuntimeHostedService> logger)
    {
        _configuration = configuration;
        _pollingService = pollingService;
        _applicationLifetime = applicationLifetime;
        _logger = logger;
    }

    public Task StartAsync(
        CancellationToken cancellationToken)
    {
        return StartPollingAsync(
            _configuration.SnmpDevices,
            cancellationToken);
    }

    public Task StopAsync(
        CancellationToken cancellationToken)
    {
        return StopPollingAsync(
            cancellationToken);
    }

    public async Task StartPollingAsync(
        IReadOnlyCollection<SnmpDeviceConfiguration> devices,
        CancellationToken cancellationToken)
    {
        ArgumentNullException.ThrowIfNull(devices);

        var plans =
            devices
                .Select(
                    SnmpConfigurationMapper.CreatePollingPlan)
                .Where(plan => plan is not null)
                .Cast<SnmpPollingPlan>()
                .ToArray();

        await _stateLock.WaitAsync(
            cancellationToken);

        try
        {
            if (_running.Count != 0)
            {
                throw new InvalidOperationException(
                    "SNMP polling is already running.");
            }

            foreach (var plan in plans)
            {
                StartPollingLocked(
                    plan);
            }

            if (plans.Length == 0)
            {
                _logger.LogInformation(
                    "No enabled SNMP devices with polling OIDs are configured.");
            }
        }
        finally
        {
            _stateLock.Release();
        }
    }

    public async Task StopPollingAsync(
        CancellationToken cancellationToken)
    {
        await _stateLock.WaitAsync(
            cancellationToken);

        try
        {
            await StopPollingLockedAsync();
        }
        finally
        {
            _stateLock.Release();
        }
    }

    private void StartPollingLocked(
        SnmpPollingPlan plan)
    {
        var cancellation =
            CancellationTokenSource.CreateLinkedTokenSource(
                _applicationLifetime.ApplicationStopping);

        var task =
            RunPollingAsync(
                plan,
                cancellation.Token);

        _running.Add(
            new RunningPollingLoop(
                cancellation,
                task));

        _logger.LogInformation(
            "Started SNMP v2c polling for {DeviceId} at {Host}:{Port}, {PointCount} OID(s), interval {PollIntervalMs} ms.",
            plan.Device.DeviceId,
            plan.Device.Host,
            plan.Device.Port,
            plan.Points.Count,
            plan.PollInterval.TotalMilliseconds);
    }

    private async Task RunPollingAsync(
        SnmpPollingPlan plan,
        CancellationToken cancellationToken)
    {
        try
        {
            await _pollingService.RunAsync(
                plan,
                cancellationToken);
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
        }
        catch (Exception exception)
        {
            _logger.LogError(
                exception,
                "SNMP polling loop for {DeviceId} terminated unexpectedly.",
                plan.Device.DeviceId);
        }
    }

    private async Task StopPollingLockedAsync()
    {
        if (_running.Count == 0)
        {
            return;
        }

        foreach (var running in _running)
        {
            running.Cancellation.Cancel();
        }

        try
        {
            await Task.WhenAll(
                _running.Select(
                    running => running.Task));
        }
        finally
        {
            foreach (var running in _running)
            {
                running.Cancellation.Dispose();
            }

            _running.Clear();
        }
    }

    private sealed record RunningPollingLoop(
        CancellationTokenSource Cancellation,
        Task Task);
}
