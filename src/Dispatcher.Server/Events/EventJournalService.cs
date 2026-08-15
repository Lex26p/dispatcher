using System.Collections.Concurrent;
using System.Text.Json;
using System.Threading.Channels;
using Dispatcher.Core.Devices;

namespace Dispatcher.Server.Events;

public sealed class EventJournalService : IHostedService
{
    private static readonly TimeSpan RetryDelay =
        TimeSpan.FromSeconds(1);

    private readonly DeviceStateService _deviceStates;
    private readonly IEventJournalStore _store;
    private readonly EventJournalOptions _options;
    private readonly ILogger<EventJournalService> _logger;
    private readonly Channel<EventRecord> _channel;
    private readonly ConcurrentDictionary<string, DeviceConnectionStatus> _lastDeviceStatuses =
        new(StringComparer.Ordinal);
    private readonly CancellationTokenSource _writerCancellation =
        new();

    private Task? _writerTask;
    private int _started;
    private long _droppedEventCount;
    private long _rejectedEventCount;

    public EventJournalService(
        DeviceStateService deviceStates,
        IEventJournalStore store,
        EventJournalOptions options,
        ILogger<EventJournalService> logger)
    {
        _deviceStates =
            deviceStates;
        _store =
            store;
        _options =
            options;
        _logger =
            logger;

        _channel =
            Channel.CreateBounded<EventRecord>(
                new BoundedChannelOptions(
                    options.BufferCapacity)
                {
                    SingleReader = true,
                    SingleWriter = false,
                    FullMode = BoundedChannelFullMode.Wait
                });
    }

    public long DroppedEventCount =>
        Interlocked.Read(
            ref _droppedEventCount);

    public long RejectedEventCount =>
        Interlocked.Read(
            ref _rejectedEventCount);

    public event Action<EventRecord>? Persisted;

    public async Task StartAsync(
        CancellationToken cancellationToken)
    {
        if (Interlocked.Exchange(
                ref _started,
                1) != 0)
        {
            throw new InvalidOperationException(
                "Event Journal is already started.");
        }

        await _store.InitializeAsync(
            cancellationToken);

        _deviceStates.Changed +=
            OnDeviceStateChanged;

        _writerTask =
            RunWriterAsync(
                _writerCancellation.Token);

        Publish(
            EventCategory.System,
            EventTypes.SystemStarted,
            EventSeverity.Information,
            source:
                "server",
            message:
                "Dispatcher started.");

        _logger.LogInformation(
            "Event Journal started with buffer capacity {BufferCapacity} and batch size {BatchSize}.",
            _options.BufferCapacity,
            _options.BatchSize);
    }

    public async Task StopAsync(
        CancellationToken cancellationToken)
    {
        if (Volatile.Read(
                ref _started) == 0)
        {
            return;
        }

        _deviceStates.Changed -=
            OnDeviceStateChanged;

        Publish(
            EventCategory.System,
            EventTypes.SystemStopping,
            EventSeverity.Information,
            source:
                "server",
            message:
                "Dispatcher is stopping.");

        _channel.Writer.TryComplete();

        if (_writerTask is null)
        {
            return;
        }

        try
        {
            await _writerTask.WaitAsync(
                cancellationToken);
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            _writerCancellation.Cancel();

            try
            {
                await _writerTask;
            }
            catch (OperationCanceledException)
                when (_writerCancellation.IsCancellationRequested)
            {
            }
        }
    }

    public bool Publish(
        EventCategory category,
        string type,
        EventSeverity severity,
        string source,
        string message,
        object? data = null,
        DateTimeOffset? timestamp = null)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            type);
        ArgumentException.ThrowIfNullOrWhiteSpace(
            source);
        ArgumentException.ThrowIfNullOrWhiteSpace(
            message);

        string? dataJson;

        try
        {
            dataJson =
                data is null
                    ? null
                    : JsonSerializer.Serialize(
                        data,
                        data.GetType());
        }
        catch (Exception exception)
        {
            var rejected =
                Interlocked.Increment(
                    ref _rejectedEventCount);

            _logger.LogWarning(
                exception,
                "Event Journal rejected event payload {RejectedEventCount} for type {EventType}.",
                rejected,
                type);

            return false;
        }

        var record =
            new EventRecord(
                EventId:
                    0,
                Timestamp:
                    (timestamp
                     ?? DateTimeOffset.UtcNow)
                    .ToUniversalTime(),
                Category:
                    category,
                Type:
                    type,
                Severity:
                    severity,
                Source:
                    source,
                Message:
                    message,
                DataJson:
                    dataJson);

        if (_channel.Writer.TryWrite(
                record))
        {
            return true;
        }

        var dropped =
            Interlocked.Increment(
                ref _droppedEventCount);

        if (dropped == 1
            || dropped % 100 == 0)
        {
            _logger.LogWarning(
                "Event Journal buffer is full. Dropped events: {DroppedEventCount}.",
                dropped);
        }

        return false;
    }

    private void OnDeviceStateChanged(
        DeviceRuntimeState state)
    {
        if (_lastDeviceStatuses.TryGetValue(
                state.DeviceId,
                out var previous)
            && previous == state.Status)
        {
            return;
        }

        _lastDeviceStatuses[
            state.DeviceId] =
            state.Status;

        switch (state.Status)
        {
            case DeviceConnectionStatus.Online:
                Publish(
                    EventCategory.Device,
                    EventTypes.DeviceOnline,
                    EventSeverity.Information,
                    state.DeviceId,
                    $"Устройство '{state.DeviceId}' Online.",
                    new
                    {
                        Status =
                            state.Status.ToString(),
                        state.LastSuccessfulPollAt
                    },
                    state.UpdatedAt);
                break;

            case DeviceConnectionStatus.Offline:
                Publish(
                    EventCategory.Device,
                    EventTypes.DeviceOffline,
                    EventSeverity.Warning,
                    state.DeviceId,
                    $"Устройство '{state.DeviceId}' Offline.",
                    new
                    {
                        Status =
                            state.Status.ToString(),
                        state.Error,
                        state.LastSuccessfulPollAt
                    },
                    state.UpdatedAt);
                break;
        }
    }

    private async Task RunWriterAsync(
        CancellationToken cancellationToken)
    {
        var batch =
            new List<EventRecord>(
                _options.BatchSize);

        try
        {
            while (await _channel.Reader.WaitToReadAsync(
                cancellationToken))
            {
                batch.Clear();

                while (batch.Count < _options.BatchSize
                       && _channel.Reader.TryRead(
                           out var record))
                {
                    batch.Add(
                        record);
                }

                if (batch.Count == 0)
                {
                    continue;
                }

                await PersistWithRetryAsync(
                    batch,
                    cancellationToken);
            }
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
        }
    }

    private void NotifyPersisted(
        EventRecord record)
    {
        var handlers =
            Persisted;

        if (handlers is null)
        {
            return;
        }

        foreach (Action<EventRecord> handler
                 in handlers.GetInvocationList())
        {
            try
            {
                handler(
                    record);
            }
            catch (Exception exception)
            {
                _logger.LogWarning(
                    exception,
                    "Event Journal persisted subscriber failed for event {EventId}.",
                    record.EventId);
            }
        }
    }

    private async Task PersistWithRetryAsync(
        IReadOnlyList<EventRecord> batch,
        CancellationToken cancellationToken)
    {
        while (true)
        {
            try
            {
                var persisted =
                    await _store.AppendEventsAsync(
                        batch,
                        cancellationToken);

                foreach (var record in persisted)
                {
                    NotifyPersisted(
                        record);
                }

                return;
            }
            catch (OperationCanceledException)
                when (cancellationToken.IsCancellationRequested)
            {
                throw;
            }
            catch (Exception exception)
            {
                _logger.LogError(
                    exception,
                    "Event Journal failed to persist a batch of {EventCount} event(s). Retrying.",
                    batch.Count);

                await Task.Delay(
                    RetryDelay,
                    cancellationToken);
            }
        }
    }
}
