using System.Threading.Channels;
using Dispatcher.Core.Tags;

namespace Dispatcher.Server.Historian;

public sealed class HistorianService : IHostedService
{
    private static readonly TimeSpan RetryDelay =
        TimeSpan.FromSeconds(1);

    private readonly TagService _tagService;
    private readonly IHistorySampleStore _store;
    private readonly HistorianOptions _options;
    private readonly ILogger<HistorianService> _logger;
    private readonly Channel<HistorySample> _channel;
    private readonly CancellationTokenSource _writerCancellation = new();

    private Task? _writerTask;
    private int _started;
    private long _droppedSampleCount;
    private long _rejectedSampleCount;

    public HistorianService(
        TagService tagService,
        IHistorySampleStore store,
        HistorianOptions options,
        ILogger<HistorianService> logger)
    {
        _tagService =
            tagService;
        _store =
            store;
        _options =
            options;
        _logger =
            logger;

        _channel =
            Channel.CreateBounded<HistorySample>(
                new BoundedChannelOptions(
                    options.BufferCapacity)
                {
                    SingleReader = true,
                    SingleWriter = false,
                    FullMode = BoundedChannelFullMode.Wait
                });
    }

    public long DroppedSampleCount =>
        Interlocked.Read(
            ref _droppedSampleCount);

    public long RejectedSampleCount =>
        Interlocked.Read(
            ref _rejectedSampleCount);

    public async Task StartAsync(
        CancellationToken cancellationToken)
    {
        if (Interlocked.Exchange(
                ref _started,
                1) != 0)
        {
            throw new InvalidOperationException(
                "Historian service is already started.");
        }

        await _store.InitializeAsync(
            cancellationToken);

        _tagService.Changed +=
            OnTagChanged;

        _writerTask =
            RunWriterAsync(
                _writerCancellation.Token);

        _logger.LogInformation(
            "Historian started with buffer capacity {BufferCapacity}, batch size {BatchSize}.",
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

        _tagService.Changed -=
            OnTagChanged;

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

    private void OnTagChanged(
        TagValue tagValue)
    {
        HistorySample sample;

        try
        {
            sample =
                HistorySampleFactory.Create(
                    tagValue);
        }
        catch (Exception exception)
        {
            var rejected =
                Interlocked.Increment(
                    ref _rejectedSampleCount);

            _logger.LogWarning(
                exception,
                "Historian rejected sample {RejectedSampleCount} for tag {TagId}.",
                rejected,
                tagValue.TagId);

            return;
        }

        if (_channel.Writer.TryWrite(
                sample))
        {
            return;
        }

        var dropped =
            Interlocked.Increment(
                ref _droppedSampleCount);

        if (dropped == 1
            || dropped % 1000 == 0)
        {
            _logger.LogWarning(
                "Historian buffer is full. Dropped samples: {DroppedSampleCount}.",
                dropped);
        }
    }

    private async Task RunWriterAsync(
        CancellationToken cancellationToken)
    {
        var batch =
            new List<HistorySample>(
                _options.BatchSize);

        try
        {
            while (await _channel.Reader.WaitToReadAsync(
                cancellationToken))
            {
                batch.Clear();

                while (batch.Count < _options.BatchSize
                       && _channel.Reader.TryRead(
                           out var sample))
                {
                    batch.Add(
                        sample);
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

    private async Task PersistWithRetryAsync(
        IReadOnlyList<HistorySample> batch,
        CancellationToken cancellationToken)
    {
        while (true)
        {
            try
            {
                await _store.AppendAsync(
                    batch,
                    cancellationToken);

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
                    "Historian failed to persist a batch of {SampleCount} sample(s). Retrying.",
                    batch.Count);

                await Task.Delay(
                    RetryDelay,
                    cancellationToken);
            }
        }
    }
}
