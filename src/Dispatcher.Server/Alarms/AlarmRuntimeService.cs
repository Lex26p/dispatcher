using System.Globalization;
using Dispatcher.Core.Tags;
using Dispatcher.Server.Configuration;
using Dispatcher.Server.Events;

namespace Dispatcher.Server.Alarms;

public sealed class AlarmRuntimeService : IHostedService, IDisposable
{
    private readonly TagService _tagService;
    private readonly AlarmDefinitionCatalog _definitions;
    private readonly ConfigurationCatalog _configuration;
    private readonly EventJournalService _eventJournal;
    private readonly ILogger<AlarmRuntimeService> _logger;
    private readonly object _gate = new();
    private readonly Dictionary<string, RuntimeEntry> _entries =
        new(StringComparer.Ordinal);

    private bool _started;
    private bool _disposed;

    public AlarmRuntimeService(
        TagService tagService,
        AlarmDefinitionCatalog definitions,
        ConfigurationCatalog configuration,
        EventJournalService eventJournal,
        ILogger<AlarmRuntimeService> logger)
    {
        _tagService = tagService;
        _definitions = definitions;
        _configuration = configuration;
        _eventJournal = eventJournal;
        _logger = logger;
    }

    public Task StartAsync(
        CancellationToken cancellationToken)
    {
        if (_disposed)
        {
            throw new ObjectDisposedException(
                nameof(AlarmRuntimeService));
        }

        lock (_gate)
        {
            if (_started)
            {
                throw new InvalidOperationException(
                    "Alarm runtime is already started.");
            }

            _started = true;
            _tagService.Changed += OnTagChanged;
            _tagService.Cleared += OnTagValuesCleared;
            _definitions.Changed += OnDefinitionsChanged;

            ReconcileDefinitionsLocked(
                evaluateCurrentValues: true);
        }

        _logger.LogInformation(
            "Alarm runtime started with {AlarmDefinitionCount} definition(s).",
            _definitions.Definitions.Count);

        return Task.CompletedTask;
    }

    public Task StopAsync(
        CancellationToken cancellationToken)
    {
        lock (_gate)
        {
            if (!_started)
            {
                return Task.CompletedTask;
            }

            _started = false;
            _tagService.Changed -= OnTagChanged;
            _tagService.Cleared -= OnTagValuesCleared;
            _definitions.Changed -= OnDefinitionsChanged;

            foreach (var entry in _entries.Values)
            {
                CancelPendingRaiseLocked(
                    entry);
            }

            _entries.Clear();
        }

        return Task.CompletedTask;
    }

    public IReadOnlyList<AlarmRuntimeSnapshot> GetAll()
    {
        lock (_gate)
        {
            return _entries.Values
                .OrderBy(
                    entry => entry.Definition.AlarmId,
                    StringComparer.Ordinal)
                .Select(
                    CreateSnapshot)
                .ToArray();
        }
    }

    public AlarmRuntimeSnapshot? Get(
        string alarmId)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            alarmId);

        lock (_gate)
        {
            return _entries.TryGetValue(
                alarmId,
                out var entry)
                ? CreateSnapshot(
                    entry)
                : null;
        }
    }


    private void OnTagValuesCleared()
    {
        lock (_gate)
        {
            if (!_started)
            {
                return;
            }

            foreach (var entry in _entries.Values)
            {
                CancelPendingRaiseLocked(
                    entry);

                if (!_configuration.ContainsTagId(
                        entry.Definition.TagId))
                {
                    entry.State =
                        AlarmRuntimeState.Normal;
                    entry.LastTransitionTimestamp =
                        null;
                }
            }
        }
    }

    public AlarmRuntimeSnapshot Acknowledge(
        string alarmId,
        EventActor? actor = null,
        DateTimeOffset? timestamp = null)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            alarmId);

        lock (_gate)
        {
            if (!_entries.TryGetValue(
                    alarmId,
                    out var entry))
            {
                throw new KeyNotFoundException(
                    $"Alarm '{alarmId}' was not found in current runtime definitions.");
            }

            var previousState =
                entry.State;

            switch (entry.State)
            {
                case AlarmRuntimeState.ActiveUnacknowledged:
                    entry.State =
                        AlarmRuntimeState.ActiveAcknowledged;
                    break;

                case AlarmRuntimeState.ReturnedUnacknowledged:
                    entry.State =
                        AlarmRuntimeState.Normal;
                    break;

                case AlarmRuntimeState.Normal:
                case AlarmRuntimeState.ActiveAcknowledged:
                    return CreateSnapshot(
                        entry);

                default:
                    throw new InvalidOperationException(
                        $"Alarm '{alarmId}' has unsupported runtime state '{entry.State}'.");
            }

            var transitionTimestamp =
                (timestamp
                 ?? DateTimeOffset.UtcNow)
                .ToUniversalTime();

            entry.LastTransitionTimestamp =
                transitionTimestamp;

            PublishTransitionLocked(
                entry,
                EventTypes.AlarmAcknowledged,
                previousState,
                entry.State,
                transitionTimestamp,
                value:
                    null,
                actor);

            return CreateSnapshot(
                entry);
        }
    }

    private void OnDefinitionsChanged()
    {
        lock (_gate)
        {
            if (!_started)
            {
                return;
            }

            ReconcileDefinitionsLocked(
                evaluateCurrentValues: true);
        }
    }

    private void OnTagChanged(
        TagValue tagValue)
    {
        lock (_gate)
        {
            if (!_started)
            {
                return;
            }

            foreach (var entry in _entries.Values)
            {
                if (string.Equals(
                        entry.Definition.TagId,
                        tagValue.TagId,
                        StringComparison.Ordinal))
                {
                    EvaluateEntryLocked(
                        entry,
                        tagValue);
                }
            }
        }
    }

    private void ReconcileDefinitionsLocked(
        bool evaluateCurrentValues)
    {
        var currentDefinitions =
            _definitions.Definitions;
        var currentIds =
            currentDefinitions
                .Select(
                    definition => definition.AlarmId)
                .ToHashSet(
                    StringComparer.Ordinal);

        foreach (var removedId in _entries.Keys
                     .Where(alarmId =>
                         !currentIds.Contains(
                             alarmId))
                     .ToArray())
        {
            var removed =
                _entries[removedId];

            CancelPendingRaiseLocked(
                removed);
            _entries.Remove(
                removedId);
        }

        foreach (var definition in currentDefinitions)
        {
            var shouldEvaluate =
                false;

            if (_entries.TryGetValue(
                    definition.AlarmId,
                    out var entry))
            {
                if (HasEvaluationSemanticsChanged(
                        entry.Definition,
                        definition))
                {
                    CancelPendingRaiseLocked(
                        entry);
                    entry.State =
                        AlarmRuntimeState.Normal;
                    entry.LastTransitionTimestamp =
                        null;
                    shouldEvaluate =
                        definition.Enabled;
                }

                entry.Definition =
                    definition;
            }
            else
            {
                entry =
                    new RuntimeEntry(
                        definition);
                _entries.Add(
                    definition.AlarmId,
                    entry);
                shouldEvaluate =
                    definition.Enabled;
            }

            if (!definition.Enabled)
            {
                CancelPendingRaiseLocked(
                    entry);
                entry.State =
                    AlarmRuntimeState.Normal;
                entry.LastTransitionTimestamp =
                    null;
                continue;
            }

            if (evaluateCurrentValues
                && shouldEvaluate)
            {
                var currentValue =
                    _tagService.Get(
                        definition.TagId);

                if (currentValue is not null)
                {
                    EvaluateEntryLocked(
                        entry,
                        currentValue,
                        raiseTimestampOverride:
                            DateTimeOffset.UtcNow);
                }
            }
        }
    }

    private void EvaluateEntryLocked(
        RuntimeEntry entry,
        TagValue tagValue,
        DateTimeOffset? raiseTimestampOverride = null)
    {
        var definition =
            entry.Definition;

        if (!definition.Enabled)
        {
            CancelPendingRaiseLocked(
                entry);
            return;
        }

        var physicallyActive =
            entry.State is
                AlarmRuntimeState.ActiveUnacknowledged
                or AlarmRuntimeState.ActiveAcknowledged;

        if (!TryEvaluateCondition(
                definition,
                tagValue.Value,
                physicallyActive,
                out var conditionActive))
        {
            CancelPendingRaiseLocked(
                entry);
            return;
        }

        if (physicallyActive)
        {
            CancelPendingRaiseLocked(
                entry);

            if (!conditionActive)
            {
                ReturnToNormalLocked(
                    entry,
                    tagValue);
            }

            return;
        }

        if (!conditionActive)
        {
            CancelPendingRaiseLocked(
                entry);
            return;
        }

        ScheduleRaiseLocked(
            entry,
            tagValue,
            raiseTimestampOverride);
    }

    private void ScheduleRaiseLocked(
        RuntimeEntry entry,
        TagValue tagValue,
        DateTimeOffset? raiseTimestampOverride)
    {
        if (entry.Definition.DelayMilliseconds == 0)
        {
            RaiseLocked(
                entry,
                tagValue,
                raiseTimestampOverride
                ?? tagValue.Timestamp);
            return;
        }

        if (entry.PendingRaise is not null)
        {
            return;
        }

        var cancellation =
            new CancellationTokenSource();
        var generation =
            ++entry.PendingGeneration;

        entry.PendingRaise =
            cancellation;

        _ = CompleteDelayedRaiseAsync(
            entry.Definition.AlarmId,
            generation,
            entry.Definition.DelayMilliseconds,
            cancellation.Token);
    }

    private async Task CompleteDelayedRaiseAsync(
        string alarmId,
        long generation,
        int delayMilliseconds,
        CancellationToken cancellationToken)
    {
        try
        {
            await Task.Delay(
                delayMilliseconds,
                cancellationToken);
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            return;
        }

        lock (_gate)
        {
            if (!_started
                || !_entries.TryGetValue(
                    alarmId,
                    out var entry)
                || entry.PendingGeneration != generation
                || entry.PendingRaise is null)
            {
                return;
            }

            entry.PendingRaise.Dispose();
            entry.PendingRaise =
                null;

            if (!entry.Definition.Enabled
                || entry.State is
                    AlarmRuntimeState.ActiveUnacknowledged
                    or AlarmRuntimeState.ActiveAcknowledged)
            {
                return;
            }

            var currentValue =
                _tagService.Get(
                    entry.Definition.TagId);

            if (currentValue is null
                || !TryEvaluateCondition(
                    entry.Definition,
                    currentValue.Value,
                    physicallyActive:
                        false,
                    out var conditionActive)
                || !conditionActive)
            {
                return;
            }

            RaiseLocked(
                entry,
                currentValue,
                DateTimeOffset.UtcNow);
        }
    }

    private void RaiseLocked(
        RuntimeEntry entry,
        TagValue tagValue,
        DateTimeOffset timestamp)
    {
        var previousState =
            entry.State;

        if (previousState is
            AlarmRuntimeState.ActiveUnacknowledged
            or AlarmRuntimeState.ActiveAcknowledged)
        {
            return;
        }

        CancelPendingRaiseLocked(
            entry);

        entry.State =
            AlarmRuntimeState.ActiveUnacknowledged;
        entry.LastTransitionTimestamp =
            timestamp.ToUniversalTime();

        PublishTransitionLocked(
            entry,
            EventTypes.AlarmRaised,
            previousState,
            entry.State,
            entry.LastTransitionTimestamp.Value,
            tagValue.Value,
            actor:
                null);
    }

    private void ReturnToNormalLocked(
        RuntimeEntry entry,
        TagValue tagValue)
    {
        var previousState =
            entry.State;

        entry.State =
            previousState == AlarmRuntimeState.ActiveAcknowledged
                ? AlarmRuntimeState.Normal
                : AlarmRuntimeState.ReturnedUnacknowledged;
        entry.LastTransitionTimestamp =
            tagValue.Timestamp.ToUniversalTime();

        PublishTransitionLocked(
            entry,
            EventTypes.AlarmReturned,
            previousState,
            entry.State,
            entry.LastTransitionTimestamp.Value,
            tagValue.Value,
            actor:
                null);
    }

    private void PublishTransitionLocked(
        RuntimeEntry entry,
        string eventType,
        AlarmRuntimeState previousState,
        AlarmRuntimeState state,
        DateTimeOffset timestamp,
        object? value,
        EventActor? actor)
    {
        var definition =
            entry.Definition;
        var accepted =
            _eventJournal.Publish(
                EventCategory.System,
                eventType,
                ToEventSeverity(
                    definition.Severity),
                source:
                    definition.AlarmId,
                message:
                    CreateTransitionMessage(
                        definition,
                        eventType),
                data:
                    new
                    {
                        definition.AlarmId,
                        definition.Name,
                        definition.TagId,
                        Condition =
                            definition.Condition.ToString(),
                        Severity =
                            definition.Severity.ToString(),
                        definition.Threshold,
                        definition.Hysteresis,
                        definition.DelayMilliseconds,
                        PreviousState =
                            previousState.ToString(),
                        State =
                            state.ToString(),
                        ValueType =
                            value?.GetType().Name,
                        ValueText =
                            FormatValue(
                                value)
                    },
                timestamp:
                    timestamp,
                actor:
                    actor);

        if (!accepted)
        {
            _logger.LogWarning(
                "Alarm transition event {EventType} for {AlarmId} could not be queued in Event Journal.",
                eventType,
                definition.AlarmId);
        }
    }

    private void CancelPendingRaiseLocked(
        RuntimeEntry entry)
    {
        var pending =
            entry.PendingRaise;

        if (pending is null)
        {
            return;
        }

        entry.PendingRaise =
            null;
        ++entry.PendingGeneration;

        pending.Cancel();
        pending.Dispose();
    }

    private static bool TryEvaluateCondition(
        AlarmDefinitionConfiguration definition,
        object? value,
        bool physicallyActive,
        out bool conditionActive)
    {
        switch (definition.Condition)
        {
            case AlarmCondition.DigitalTrue:
            case AlarmCondition.DigitalFalse:
                if (!TryGetDigitalValue(
                        value,
                        out var digitalValue))
                {
                    conditionActive =
                        false;
                    return false;
                }

                conditionActive =
                    definition.Condition == AlarmCondition.DigitalTrue
                        ? digitalValue
                        : !digitalValue;
                return true;

            case AlarmCondition.High:
            case AlarmCondition.Low:
                if (!TryGetDecimalValue(
                        value,
                        out var numericValue))
                {
                    conditionActive =
                        false;
                    return false;
                }

                var threshold =
                    definition.Threshold
                    ?? throw new InvalidOperationException(
                        $"Alarm '{definition.AlarmId}' does not define a threshold.");
                var hysteresis =
                    definition.Hysteresis
                    ?? throw new InvalidOperationException(
                        $"Alarm '{definition.AlarmId}' does not define hysteresis.");

                if (definition.Condition == AlarmCondition.High)
                {
                    var clearThreshold =
                        SaturatingSubtract(
                            threshold,
                            hysteresis);

                    conditionActive =
                        physicallyActive
                            ? numericValue >= clearThreshold
                            : numericValue >= threshold;
                    return true;
                }

                var lowClearThreshold =
                    SaturatingAdd(
                        threshold,
                        hysteresis);

                conditionActive =
                    physicallyActive
                        ? numericValue <= lowClearThreshold
                        : numericValue <= threshold;
                return true;

            default:
                throw new InvalidOperationException(
                    $"Alarm '{definition.AlarmId}' has unsupported condition '{definition.Condition}'.");
        }
    }

    private static bool TryGetDigitalValue(
        object? value,
        out bool result)
    {
        if (value is bool booleanValue)
        {
            result =
                booleanValue;
            return true;
        }

        if (TryGetDecimalValue(
                value,
                out var numericValue))
        {
            result =
                numericValue != 0m;
            return true;
        }

        result =
            false;
        return false;
    }

    private static bool TryGetDecimalValue(
        object? value,
        out decimal result)
    {
        try
        {
            switch (value)
            {
                case decimal decimalValue:
                    result = decimalValue;
                    return true;

                case byte or sbyte or short or ushort or int or uint or long or ulong:
                    result = Convert.ToDecimal(
                        value,
                        CultureInfo.InvariantCulture);
                    return true;

                case float floatValue when float.IsFinite(floatValue):
                    result = Convert.ToDecimal(
                        floatValue);
                    return true;

                case double doubleValue when double.IsFinite(doubleValue):
                    result = Convert.ToDecimal(
                        doubleValue);
                    return true;

                default:
                    result =
                        default;
                    return false;
            }
        }
        catch (OverflowException)
        {
            result =
                default;
            return false;
        }
    }

    private static decimal SaturatingSubtract(
        decimal value,
        decimal amount)
    {
        return value < decimal.MinValue + amount
            ? decimal.MinValue
            : value - amount;
    }

    private static decimal SaturatingAdd(
        decimal value,
        decimal amount)
    {
        return value > decimal.MaxValue - amount
            ? decimal.MaxValue
            : value + amount;
    }

    private static bool HasEvaluationSemanticsChanged(
        AlarmDefinitionConfiguration previous,
        AlarmDefinitionConfiguration current)
    {
        return previous.Enabled != current.Enabled
            || !string.Equals(
                previous.TagId,
                current.TagId,
                StringComparison.Ordinal)
            || previous.Condition != current.Condition
            || previous.Threshold != current.Threshold
            || previous.DelayMilliseconds != current.DelayMilliseconds
            || previous.Hysteresis != current.Hysteresis;
    }

    private static EventSeverity ToEventSeverity(
        AlarmSeverity severity)
    {
        return severity switch
        {
            AlarmSeverity.Information =>
                EventSeverity.Information,
            AlarmSeverity.Warning =>
                EventSeverity.Warning,
            AlarmSeverity.Error =>
                EventSeverity.Error,
            _ =>
                throw new ArgumentOutOfRangeException(
                    nameof(severity),
                    severity,
                    null)
        };
    }

    private static string CreateTransitionMessage(
        AlarmDefinitionConfiguration definition,
        string eventType)
    {
        return eventType switch
        {
            EventTypes.AlarmRaised =>
                $"Alarm '{definition.AlarmId}' raised: {definition.Message}",
            EventTypes.AlarmReturned =>
                $"Alarm '{definition.AlarmId}' returned to normal.",
            EventTypes.AlarmAcknowledged =>
                $"Alarm '{definition.AlarmId}' acknowledged.",
            _ =>
                $"Alarm '{definition.AlarmId}' changed state."
        };
    }

    private static string? FormatValue(
        object? value)
    {
        return value switch
        {
            null =>
                null,
            IFormattable formattable =>
                formattable.ToString(
                    null,
                    CultureInfo.InvariantCulture),
            _ =>
                value.ToString()
        };
    }

    private static AlarmRuntimeSnapshot CreateSnapshot(
        RuntimeEntry entry)
    {
        var definition =
            entry.Definition;

        return new AlarmRuntimeSnapshot(
            definition.AlarmId,
            definition.Name,
            definition.TagId,
            definition.Severity,
            definition.Message,
            entry.State,
            entry.LastTransitionTimestamp);
    }

    public void Dispose()
    {
        lock (_gate)
        {
            if (_disposed)
            {
                return;
            }

            if (_started)
            {
                _tagService.Changed -= OnTagChanged;
                _tagService.Cleared -= OnTagValuesCleared;
                _definitions.Changed -= OnDefinitionsChanged;
                _started = false;
            }

            foreach (var entry in _entries.Values)
            {
                CancelPendingRaiseLocked(
                    entry);
            }

            _entries.Clear();
            _disposed =
                true;
        }
    }

    private sealed class RuntimeEntry
    {
        public RuntimeEntry(
            AlarmDefinitionConfiguration definition)
        {
            Definition =
                definition;
        }

        public AlarmDefinitionConfiguration Definition { get; set; }

        public AlarmRuntimeState State { get; set; } =
            AlarmRuntimeState.Normal;

        public DateTimeOffset? LastTransitionTimestamp { get; set; }

        public CancellationTokenSource? PendingRaise { get; set; }

        public long PendingGeneration { get; set; }
    }
}
