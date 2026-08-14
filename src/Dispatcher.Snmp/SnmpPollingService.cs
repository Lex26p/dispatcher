using Dispatcher.Core.Devices;
using Dispatcher.Core.Tags;
using Dispatcher.Snmp.Configuration;

namespace Dispatcher.Snmp;

public sealed class SnmpPollingService
{
    private readonly TagService _tagService;
    private readonly DeviceStateService _deviceStateService;
    private readonly SnmpGetClient _client;

    public SnmpPollingService(
        TagService tagService,
        DeviceStateService deviceStateService,
        SnmpGetClient client)
    {
        _tagService = tagService;
        _deviceStateService = deviceStateService;
        _client = client;
    }

    public async Task<DeviceRuntimeState> PollOnceAsync(
        SnmpPollingPlan plan,
        CancellationToken cancellationToken = default)
    {
        ValidatePlan(plan);

        try
        {
            var values = await _client.ReadAsync(
                plan.Device,
                plan.Points,
                plan.RequestTimeout,
                cancellationToken);

            var timestamp = DateTimeOffset.UtcNow;

            foreach (var point in plan.Points)
            {
                _tagService.Set(
                    point.TagId,
                    values[point.TagId],
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
        SnmpPollingPlan plan,
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

    private static void ValidatePlan(SnmpPollingPlan plan)
    {
        ArgumentNullException.ThrowIfNull(plan);
        ArgumentNullException.ThrowIfNull(plan.Device);
        ArgumentNullException.ThrowIfNull(plan.Points);
        ArgumentException.ThrowIfNullOrWhiteSpace(plan.Device.DeviceId);

        if (plan.Points.Count == 0)
        {
            throw new ArgumentException(
                "At least one SNMP polling point is required.",
                nameof(plan));
        }

        foreach (var point in plan.Points)
        {
            ArgumentNullException.ThrowIfNull(point);
            ArgumentException.ThrowIfNullOrWhiteSpace(point.TagId);
            SnmpOidValidator.Validate(point.Oid);
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
