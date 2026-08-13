using Dispatcher.Modbus;
using Dispatcher.Server.Configuration;
using Microsoft.Extensions.Options;

namespace Dispatcher.Server.Runtime;

public sealed class ModbusRuntimeHostedService : BackgroundService
{
    private readonly IOptions<ModbusRuntimeOptions> _options;
    private readonly ModbusPollingService _pollingService;
    private readonly ILogger<ModbusRuntimeHostedService> _logger;

    public ModbusRuntimeHostedService(
        IOptions<ModbusRuntimeOptions> options,
        ModbusPollingService pollingService,
        ILogger<ModbusRuntimeHostedService> logger)
    {
        _options = options;
        _pollingService = pollingService;
        _logger = logger;
    }

    protected override async Task ExecuteAsync(
        CancellationToken stoppingToken)
    {
        var options = _options.Value;

        if (!options.Enabled)
        {
            _logger.LogInformation(
                "Modbus runtime is disabled.");

            return;
        }

        var plan = options.CreatePollingPlan();

        _logger.LogInformation(
            "Starting Modbus polling for {DeviceId} at {Host}:{Port}, UnitId {UnitId}, {PointCount} point(s), interval {PollIntervalMs} ms.",
            plan.Device.DeviceId,
            plan.Device.Host,
            plan.Device.Port,
            plan.Device.UnitId,
            plan.Points.Count,
            plan.PollInterval.TotalMilliseconds);

        await _pollingService.RunAsync(
            plan,
            stoppingToken);
    }
}
