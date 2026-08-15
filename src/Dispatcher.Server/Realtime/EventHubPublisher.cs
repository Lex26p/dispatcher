using Dispatcher.Contracts.Realtime;
using Dispatcher.Server.Events;
using Microsoft.AspNetCore.SignalR;

namespace Dispatcher.Server.Realtime;

public sealed class EventHubPublisher : IHostedService
{
    private readonly EventJournalService _eventJournal;
    private readonly IHubContext<RuntimeHub> _hubContext;
    private readonly ILogger<EventHubPublisher> _logger;

    public EventHubPublisher(
        EventJournalService eventJournal,
        IHubContext<RuntimeHub> hubContext,
        ILogger<EventHubPublisher> logger)
    {
        _eventJournal =
            eventJournal;
        _hubContext =
            hubContext;
        _logger =
            logger;
    }

    public Task StartAsync(
        CancellationToken cancellationToken)
    {
        _eventJournal.Persisted +=
            OnEventPersisted;

        return Task.CompletedTask;
    }

    public Task StopAsync(
        CancellationToken cancellationToken)
    {
        _eventJournal.Persisted -=
            OnEventPersisted;

        return Task.CompletedTask;
    }

    private async void OnEventPersisted(
        EventRecord record)
    {
        try
        {
            await _hubContext.Clients.All.SendAsync(
                RuntimeHubContract.EventAdded,
                EventContractMapper.ToDto(
                    record));
        }
        catch (Exception exception)
        {
            _logger.LogWarning(
                exception,
                "Failed to publish persisted event {EventId} through SignalR.",
                record.EventId);
        }
    }
}
