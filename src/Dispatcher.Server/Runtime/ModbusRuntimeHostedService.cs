using Dispatcher.Modbus;
using Dispatcher.Server.Configuration;

namespace Dispatcher.Server.Runtime;

public sealed class ModbusRuntimeHostedService : BackgroundService
{
    private readonly ConfigurationCatalog _configuration;
    private readonly ModbusPollingService _pollingService;
    private readonly ILogger<ModbusRuntimeHostedService> _logger;

    public ModbusRuntimeHostedService(
        ConfigurationCatalog configuration,
        ModbusPollingService pollingService,
        ILogger<ModbusRuntimeHostedService> logger)
    {
        _configuration = configuration;
        _pollingService = pollingService;
        _logger = logger;
    }

    protected override async Task ExecuteAsync(
        CancellationToken stoppingToken)
    {
        var pollingTasks =
            new List<Task>();

        foreach (var device in _configuration.Devices)
        {
            if (!device.Enabled)
            {
                continue;
            }

            var plan =
                ModbusConfigurationMapper.CreatePollingPlan(
                    device);

            if (plan is null)
            {
                _logger.LogWarning(
                    "Skipping enabled Modbus device {DeviceId} because it has no configured tags.",
                    device.DeviceId);

                continue;
            }

            _logger.LogInformation(
                "Starting Modbus polling for {DeviceId} at {Host}:{Port}, UnitId {UnitId}, {PointCount} point(s), interval {PollIntervalMs} ms.",
                plan.Device.DeviceId,
                plan.Device.Host,
                plan.Device.Port,
                plan.Device.UnitId,
                plan.Points.Count,
                plan.PollInterval.TotalMilliseconds);

            pollingTasks.Add(
                _pollingService.RunAsync(
                    plan,
                    stoppingToken));
        }

        if (pollingTasks.Count == 0)
        {
            _logger.LogInformation(
                "No enabled Modbus devices with polling tags are configured.");

            return;
        }

        await Task.WhenAll(
            pollingTasks);
    }
}
