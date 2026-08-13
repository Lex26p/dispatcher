using Dispatcher.Core.Devices;
using Dispatcher.Core.Tags;
using Dispatcher.Modbus.Configuration;

namespace Dispatcher.Modbus;

public sealed class ModbusPollingService
{
    private readonly TagService _tagService;
    private readonly DeviceStateService _deviceStateService;
    private readonly ModbusTcpRegisterReader _reader;

    public ModbusPollingService(
        TagService tagService,
        DeviceStateService deviceStateService,
        ModbusTcpRegisterReader reader)
    {
        _tagService = tagService;
        _deviceStateService = deviceStateService;
        _reader = reader;
    }

    public async Task<DeviceRuntimeState> PollOnceAsync(
        ModbusPollingPlan plan,
        CancellationToken cancellationToken = default)
    {
        ValidatePlan(plan);

        try
        {
            var values = await _reader.ReadHoldingRegistersAsync(
                plan.Device,
                plan.Points.Select(point => point.Address).ToArray(),
                plan.RequestTimeout,
                cancellationToken);

            var timestamp = DateTimeOffset.UtcNow;

            foreach (var point in plan.Points)
            {
                _tagService.Set(
                    point.TagId,
                    values[point.Address],
                    timestamp);
            }

            return _deviceStateService.SetOnline(
                plan.Device.DeviceId,
                timestamp);
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception exception)
        {
            return _deviceStateService.SetOffline(
                plan.Device.DeviceId,
                exception.Message,
                DateTimeOffset.UtcNow);
        }
    }

    public async Task RunAsync(
        ModbusPollingPlan plan,
        CancellationToken cancellationToken)
    {
        ValidatePlan(plan);

        while (!cancellationToken.IsCancellationRequested)
        {
            try
            {
                await PollOnceAsync(plan, cancellationToken);
            }
            catch (OperationCanceledException)
                when (cancellationToken.IsCancellationRequested)
            {
                break;
            }

            try
            {
                await Task.Delay(
                    plan.PollInterval,
                    cancellationToken);
            }
            catch (OperationCanceledException)
                when (cancellationToken.IsCancellationRequested)
            {
                break;
            }
        }
    }

    private static void ValidatePlan(ModbusPollingPlan plan)
    {
        ArgumentNullException.ThrowIfNull(plan);
        ArgumentNullException.ThrowIfNull(plan.Device);
        ArgumentNullException.ThrowIfNull(plan.Points);

        ArgumentException.ThrowIfNullOrWhiteSpace(plan.Device.DeviceId);

        if (plan.Points.Count == 0)
        {
            throw new ArgumentException(
                "At least one Modbus polling point is required.",
                nameof(plan));
        }

        foreach (var point in plan.Points)
        {
            ArgumentNullException.ThrowIfNull(point);
            ArgumentException.ThrowIfNullOrWhiteSpace(point.TagId);
        }

        if (plan.PollInterval <= TimeSpan.Zero)
        {
            throw new ArgumentOutOfRangeException(
                nameof(plan),
                plan.PollInterval,
                "Polling interval must be greater than zero.");
        }

        if (plan.RequestTimeout <= TimeSpan.Zero)
        {
            throw new ArgumentOutOfRangeException(
                nameof(plan),
                plan.RequestTimeout,
                "Request timeout must be greater than zero.");
        }
    }
}
