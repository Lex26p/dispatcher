using System.Collections.Concurrent;
using System.Threading.Channels;
using Dispatcher.Core.Tags;
using Dispatcher.Server.Configuration;

namespace Dispatcher.Server.Historian;

public sealed class HistorianService : IHostedService
{
    private static readonly TimeSpan RetryDelay =
        TimeSpan.FromSeconds(1);

    private readonly TagService _tagService;
    private readonly IHistorySampleStore _store;
    private readonly ConfigurationCatalog _configuration;
    private readonly HistorianPolicyCatalog _policies;
    private readonly HistorianOptions _options;
    private readonly ILogger<HistorianService> _logger;
    private readonly Channel<HistorySample> _channel;
    private readonly ConcurrentDictionary<string, DateTimeOffset> _nextPeriodicDue =
        new(StringComparer.Ordinal);
    private readonly ConcurrentDictionary<string, int> _periodicIntervals =
        new(StringComparer.Ordinal);
    private readonly CancellationTokenSource _writerCancellation = new();
    private readonly CancellationTokenSource _periodicCancellation = new();

    private Task? _writerTask;
    private Task? _periodicTask;
    private int _started;
    private long _droppedSampleCount;
    private long _rejectedSampleCount;

    public HistorianService(
        TagService tagService,
        IHistorySampleStore store,
        ConfigurationCatalog configuration,
        HistorianPolicyCatalog policies,
        HistorianOptions options,
        ILogger<HistorianService> logger)
    {
        _tagService =
            tagService;
        _store =
            store;
        _configuration =
            configuration;
        _policies =
            policies;
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

        _periodicTask =
            RunPeriodicSamplerAsync(
                _periodicCancellation.Token);

        _logger.LogInformation(
            "Historian started with {PolicyCount} policy/policies, buffer capacity {BufferCapacity}, batch size {BatchSize}, and periodic scan {PeriodicScanMilliseconds} ms.",
            _policies.Policies.Count,
            _options.BufferCapacity,
            _options.BatchSize,
            _options.PeriodicScanMilliseconds);
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

        _periodicCancellation.Cancel();

        if (_periodicTask is not null)
        {
            try
            {
                await _periodicTask;
            }
            catch (OperationCanceledException)
                when (_periodicCancellation.IsCancellationRequested)
            {
            }
        }

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
        var policy =
            _policies.Find(
                tagValue.TagId);

        if (policy is not
            {
                Enabled: true,
                Mode: HistorianSamplingMode.OnChange
            }
            || !_configuration.ContainsTagId(
                tagValue.TagId))
        {
            return;
        }

        TryEnqueue(
            tagValue);
    }

    private async Task RunPeriodicSamplerAsync(
        CancellationToken cancellationToken)
    {
        var scanDelay =
            TimeSpan.FromMilliseconds(
                _options.PeriodicScanMilliseconds);

        try
        {
            while (true)
            {
                cancellationToken.ThrowIfCancellationRequested();

                var now =
                    DateTimeOffset.UtcNow;

                var activeTagIds =
                    new HashSet<string>(
                        StringComparer.Ordinal);

                foreach (var policy in _policies.Policies)
                {
                    if (!policy.Enabled
                        || policy.Mode != HistorianSamplingMode.Periodic
                        || policy.PeriodMilliseconds is null)
                    {
                        continue;
                    }

                    if (!_configuration.ContainsTagId(
                            policy.TagId))
                    {
                        continue;
                    }

                    activeTagIds.Add(
                        policy.TagId);

                    var periodMilliseconds =
                        policy.PeriodMilliseconds.Value;

                    if (!_periodicIntervals.TryGetValue(
                            policy.TagId,
                            out var previousPeriod)
                        || previousPeriod != periodMilliseconds)
                    {
                        _periodicIntervals[
                            policy.TagId] =
                            periodMilliseconds;

                        _nextPeriodicDue[
                            policy.TagId] =
                            now;
                    }

                    var nextDue =
                        _nextPeriodicDue.GetOrAdd(
                            policy.TagId,
                            now);

                    if (now < nextDue)
                    {
                        continue;
                    }

                    _nextPeriodicDue[
                        policy.TagId] =
                        now.AddMilliseconds(
                            periodMilliseconds);

                    var current =
                        _tagService.Get(
                            policy.TagId);

                    if (current is null)
                    {
                        continue;
                    }

                    TryEnqueue(
                        new TagValue(
                            current.TagId,
                            current.Value,
                            now));
                }

                foreach (var tagId in _nextPeriodicDue.Keys)
                {
                    if (!activeTagIds.Contains(
                            tagId))
                    {
                        _nextPeriodicDue.TryRemove(
                            tagId,
                            out _);
                        _periodicIntervals.TryRemove(
                            tagId,
                            out _);
                    }
                }

                await Task.Delay(
                    scanDelay,
                    cancellationToken);
            }
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
        }
    }

    private void TryEnqueue(
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
