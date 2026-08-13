using Dispatcher.Core.Devices;
using Dispatcher.Core.Tags;
using Dispatcher.Modbus;
using Dispatcher.Modbus.Configuration;
using Dispatcher.Server.Configuration;

namespace Dispatcher.Server.Runtime;

public sealed class ModbusRuntimeHostedService : IHostedService
{
    private readonly ConfigurationCatalog _configuration;
    private readonly ModbusPollingService _pollingService;
    private readonly TagService _tagService;
    private readonly DeviceStateService _deviceStateService;
    private readonly IHostApplicationLifetime _applicationLifetime;
    private readonly ILogger<ModbusRuntimeHostedService> _logger;
    private readonly SemaphoreSlim _applyLock = new(1, 1);
    private readonly List<RunningPollingLoop> _running = [];

    public ModbusRuntimeHostedService(
        ConfigurationCatalog configuration,
        ModbusPollingService pollingService,
        TagService tagService,
        DeviceStateService deviceStateService,
        IHostApplicationLifetime applicationLifetime,
        ILogger<ModbusRuntimeHostedService> logger)
    {
        _configuration = configuration;
        _pollingService = pollingService;
        _tagService = tagService;
        _deviceStateService = deviceStateService;
        _applicationLifetime = applicationLifetime;
        _logger = logger;
    }

    public async Task StartAsync(
        CancellationToken cancellationToken)
    {
        await ApplyAsync(
            _configuration.Devices,
            cancellationToken);
    }

    public async Task StopAsync(
        CancellationToken cancellationToken)
    {
        await _applyLock.WaitAsync(
            cancellationToken);

        try
        {
            await StopPollingLockedAsync();
        }
        finally
        {
            _applyLock.Release();
        }
    }

    public async Task ApplyAsync(
        IReadOnlyCollection<ModbusDeviceConfiguration> devices,
        CancellationToken cancellationToken)
    {
        ArgumentNullException.ThrowIfNull(devices);

        var plans =
            devices
                .Select(
                    ModbusConfigurationMapper.CreatePollingPlan)
                .Where(plan => plan is not null)
                .Cast<ModbusPollingPlan>()
                .ToArray();

        await _applyLock.WaitAsync(
            cancellationToken);

        try
        {
            await StopPollingLockedAsync();

            _tagService.Clear();
            _deviceStateService.Clear();

            foreach (var plan in plans)
            {
                StartPollingLocked(
                    plan);
            }

            if (plans.Length == 0)
            {
                _logger.LogInformation(
                    "No enabled Modbus devices with polling tags are configured.");
            }
        }
        finally
        {
            _applyLock.Release();
        }
    }

    private void StartPollingLocked(
        ModbusPollingPlan plan)
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
            "Started Modbus polling for {DeviceId} at {Host}:{Port}, UnitId {UnitId}, {PointCount} point(s), interval {PollIntervalMs} ms.",
            plan.Device.DeviceId,
            plan.Device.Host,
            plan.Device.Port,
            plan.Device.UnitId,
            plan.Points.Count,
            plan.PollInterval.TotalMilliseconds);
    }

    private async Task RunPollingAsync(
        ModbusPollingPlan plan,
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
                "Modbus polling loop for {DeviceId} terminated unexpectedly.",
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
