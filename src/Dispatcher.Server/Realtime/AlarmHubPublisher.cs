using Dispatcher.Contracts.Realtime;
using Dispatcher.Server.Alarms;
using Microsoft.AspNetCore.SignalR;

namespace Dispatcher.Server.Realtime;

public sealed class AlarmHubPublisher : IHostedService
{
    private readonly AlarmRuntimeService _runtime;
    private readonly IHubContext<RuntimeHub> _hubContext;
    private readonly ILogger<AlarmHubPublisher> _logger;

    public AlarmHubPublisher(
        AlarmRuntimeService runtime,
        IHubContext<RuntimeHub> hubContext,
        ILogger<AlarmHubPublisher> logger)
    {
        _runtime =
            runtime;
        _hubContext =
            hubContext;
        _logger =
            logger;
    }

    public Task StartAsync(
        CancellationToken cancellationToken)
    {
        _runtime.Changed +=
            OnAlarmChanged;

        return Task.CompletedTask;
    }

    public Task StopAsync(
        CancellationToken cancellationToken)
    {
        _runtime.Changed -=
            OnAlarmChanged;

        return Task.CompletedTask;
    }

    private async void OnAlarmChanged(
        AlarmRuntimeSnapshot snapshot)
    {
        try
        {
            await _hubContext.Clients.All.SendAsync(
                RuntimeHubContract.AlarmChanged,
                AlarmRuntimeContractMapper.ToDto(
                    snapshot));
        }
        catch (Exception exception)
        {
            _logger.LogWarning(
                exception,
                "Failed to publish alarm runtime change for {AlarmId} through SignalR.",
                snapshot.AlarmId);
        }
    }
}
