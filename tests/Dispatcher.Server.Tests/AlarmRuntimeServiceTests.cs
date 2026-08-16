using Dispatcher.Core.Devices;
using Dispatcher.Core.Tags;
using Dispatcher.Server.Alarms;
using Dispatcher.Server.Configuration;
using Dispatcher.Server.Events;
using Dispatcher.Server.Historian;
using Microsoft.Extensions.Logging.Abstractions;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class AlarmRuntimeServiceTests
{
    [TestMethod]
    public async Task Runtime_LifecycleSupportsRaiseAckReturnAndAckAfterReturn()
    {
        using var environment =
            await RuntimeEnvironment.CreateAsync(
                [
                    CreateHighDefinition(
                        alarmId:
                            "alarm.high",
                        tagId:
                            "tag.high",
                        threshold:
                            10m,
                        hysteresis:
                            2m)
                ]);

        var t0 =
            new DateTimeOffset(
                2026,
                8,
                16,
                18,
                0,
                0,
                TimeSpan.Zero);

        environment.Tags.Set(
            "tag.high",
            10m,
            t0);

        AssertState(
            environment.Runtime,
            "alarm.high",
            AlarmRuntimeState.ActiveUnacknowledged);

        environment.Tags.Set(
            "tag.high",
            8m,
            t0.AddSeconds(1));

        AssertState(
            environment.Runtime,
            "alarm.high",
            AlarmRuntimeState.ActiveUnacknowledged);

        var actor =
            new EventActor(
                "operator-01",
                "operator.one");

        environment.Runtime.Acknowledge(
            "alarm.high",
            actor,
            t0.AddSeconds(2));

        AssertState(
            environment.Runtime,
            "alarm.high",
            AlarmRuntimeState.ActiveAcknowledged);

        environment.Tags.Set(
            "tag.high",
            7.9m,
            t0.AddSeconds(3));

        AssertState(
            environment.Runtime,
            "alarm.high",
            AlarmRuntimeState.Normal);

        environment.Tags.Set(
            "tag.high",
            11m,
            t0.AddSeconds(4));

        AssertState(
            environment.Runtime,
            "alarm.high",
            AlarmRuntimeState.ActiveUnacknowledged);

        environment.Tags.Set(
            "tag.high",
            7m,
            t0.AddSeconds(5));

        AssertState(
            environment.Runtime,
            "alarm.high",
            AlarmRuntimeState.ReturnedUnacknowledged);

        environment.Runtime.Acknowledge(
            "alarm.high",
            actor,
            t0.AddSeconds(6));

        AssertState(
            environment.Runtime,
            "alarm.high",
            AlarmRuntimeState.Normal);

        await environment.StopAsync();

        var alarmEvents =
            (await environment.Store.LoadAllEventsAsync())
                .Where(record =>
                    record.Type is
                        EventTypes.AlarmRaised
                        or EventTypes.AlarmAcknowledged
                        or EventTypes.AlarmReturned)
                .ToArray();

        CollectionAssert.AreEqual(
            new[]
            {
                EventTypes.AlarmRaised,
                EventTypes.AlarmAcknowledged,
                EventTypes.AlarmReturned,
                EventTypes.AlarmRaised,
                EventTypes.AlarmReturned,
                EventTypes.AlarmAcknowledged
            },
            alarmEvents
                .Select(record => record.Type)
                .ToArray());

        Assert.IsTrue(
            alarmEvents.All(record =>
                record.Category == EventCategory.System
                && record.Source == "alarm.high"
                && record.Severity == EventSeverity.Warning));

        var acknowledgeEvents =
            alarmEvents
                .Where(record =>
                    record.Type == EventTypes.AlarmAcknowledged)
                .ToArray();

        Assert.IsTrue(
            acknowledgeEvents.All(record =>
                record.ActorUserId == actor.UserId
                && record.ActorUserName == actor.UserName));

        Assert.IsTrue(
            alarmEvents
                .Where(record =>
                    record.Type != EventTypes.AlarmAcknowledged)
                .All(record =>
                    record.ActorUserId is null
                    && record.ActorUserName is null));

        StringAssert.Contains(
            alarmEvents[0].DataJson
            ?? string.Empty,
            "ActiveUnacknowledged");
    }

    [TestMethod]
    public async Task Runtime_EvaluatesLowAndDigitalConditions()
    {
        using var environment =
            await RuntimeEnvironment.CreateAsync(
                [
                    new AlarmDefinitionConfiguration(
                        AlarmId:
                            "alarm.low",
                        Name:
                            "Low",
                        Enabled:
                            true,
                        TagId:
                            "tag.low",
                        Condition:
                            AlarmCondition.Low,
                        Threshold:
                            5m,
                        Severity:
                            AlarmSeverity.Warning,
                        Message:
                            "Low value",
                        DelayMilliseconds:
                            0,
                        Hysteresis:
                            1m),
                    CreateDigitalDefinition(
                        "alarm.true",
                        "tag.true",
                        AlarmCondition.DigitalTrue),
                    CreateDigitalDefinition(
                        "alarm.false",
                        "tag.false",
                        AlarmCondition.DigitalFalse)
                ]);

        environment.Tags.Set(
            "tag.low",
            5m);
        AssertState(
            environment.Runtime,
            "alarm.low",
            AlarmRuntimeState.ActiveUnacknowledged);

        environment.Tags.Set(
            "tag.low",
            6m);
        AssertState(
            environment.Runtime,
            "alarm.low",
            AlarmRuntimeState.ActiveUnacknowledged);

        environment.Tags.Set(
            "tag.low",
            6.1m);
        AssertState(
            environment.Runtime,
            "alarm.low",
            AlarmRuntimeState.ReturnedUnacknowledged);

        environment.Tags.Set(
            "tag.true",
            0);
        AssertState(
            environment.Runtime,
            "alarm.true",
            AlarmRuntimeState.Normal);

        environment.Tags.Set(
            "tag.true",
            2);
        AssertState(
            environment.Runtime,
            "alarm.true",
            AlarmRuntimeState.ActiveUnacknowledged);

        environment.Tags.Set(
            "tag.false",
            true);
        AssertState(
            environment.Runtime,
            "alarm.false",
            AlarmRuntimeState.Normal);

        environment.Tags.Set(
            "tag.false",
            0u);
        AssertState(
            environment.Runtime,
            "alarm.false",
            AlarmRuntimeState.ActiveUnacknowledged);
    }

    [TestMethod]
    public async Task Runtime_DelayRequiresContinuouslyActiveCondition()
    {
        using var environment =
            await RuntimeEnvironment.CreateAsync(
                [
                    CreateHighDefinition(
                        alarmId:
                            "alarm.delayed",
                        tagId:
                            "tag.delayed",
                        threshold:
                            10m,
                        hysteresis:
                            0m,
                        delayMilliseconds:
                            100)
                ]);

        environment.Tags.Set(
            "tag.delayed",
            11m);

        await Task.Delay(
            30);

        environment.Tags.Set(
            "tag.delayed",
            9m);

        await Task.Delay(
            130);

        AssertState(
            environment.Runtime,
            "alarm.delayed",
            AlarmRuntimeState.Normal);

        environment.Tags.Set(
            "tag.delayed",
            11m);

        await WaitForStateAsync(
            environment.Runtime,
            "alarm.delayed",
            AlarmRuntimeState.ActiveUnacknowledged);

        await environment.StopAsync();

        var raised =
            (await environment.Store.LoadAllEventsAsync())
                .Where(record =>
                    record.Type == EventTypes.AlarmRaised
                    && record.Source == "alarm.delayed")
                .ToArray();

        Assert.AreEqual(
            1,
            raised.Length);
    }

    [TestMethod]
    public async Task Runtime_CatalogChangesReevaluateCurrentValueAndPreserveMetadataOnlyState()
    {
        using var environment =
            await RuntimeEnvironment.CreateAsync(
                Array.Empty<AlarmDefinitionConfiguration>());

        environment.Tags.Set(
            "tag.current",
            11m);

        var original =
            CreateHighDefinition(
                alarmId:
                    "alarm.current",
                tagId:
                    "tag.current",
                threshold:
                    10m,
                hysteresis:
                    1m);

        environment.Definitions.ReplaceAll(
            [
                original
            ]);

        AssertState(
            environment.Runtime,
            "alarm.current",
            AlarmRuntimeState.ActiveUnacknowledged);

        environment.Runtime.Acknowledge(
            "alarm.current");

        environment.Definitions.ReplaceAll(
            [
                original with
                {
                    Name =
                        "Renamed current alarm",
                    Message =
                        "Updated message",
                    Severity =
                        AlarmSeverity.Error
                }
            ]);

        var metadataUpdated =
            environment.Runtime.Get(
                "alarm.current")
            ?? throw new InvalidOperationException(
                "Alarm runtime state was not found.");

        Assert.AreEqual(
            AlarmRuntimeState.ActiveAcknowledged,
            metadataUpdated.State);
        Assert.AreEqual(
            "Renamed current alarm",
            metadataUpdated.Name);
        Assert.AreEqual(
            AlarmSeverity.Error,
            metadataUpdated.Severity);

        environment.Definitions.ReplaceAll(
            [
                original with
                {
                    Threshold =
                        20m
                }
            ]);

        AssertState(
            environment.Runtime,
            "alarm.current",
            AlarmRuntimeState.Normal);

        environment.Definitions.ReplaceAll(
            [
                original
            ]);

        AssertState(
            environment.Runtime,
            "alarm.current",
            AlarmRuntimeState.ActiveUnacknowledged);

        environment.Configuration.ReplaceAll(
            [
                CreateConfiguredDevice(
                    "tag.current")
            ],
            Array.Empty<SnmpDeviceConfiguration>());

        environment.Tags.Clear();

        AssertState(
            environment.Runtime,
            "alarm.current",
            AlarmRuntimeState.ActiveUnacknowledged);

        environment.Configuration.ReplaceAll(
            Array.Empty<ModbusDeviceConfiguration>(),
            Array.Empty<SnmpDeviceConfiguration>());

        environment.Tags.Clear();

        AssertState(
            environment.Runtime,
            "alarm.current",
            AlarmRuntimeState.Normal);
    }

    private static ModbusDeviceConfiguration CreateConfiguredDevice(
        string tagId)
    {
        return new ModbusDeviceConfiguration(
            DeviceId:
                "alarm-runtime-device",
            Name:
                "Alarm runtime device",
            Enabled:
                false,
            Host:
                "127.0.0.1",
            Port:
                502,
            UnitId:
                1,
            PollIntervalMilliseconds:
                1000,
            RequestTimeoutMilliseconds:
                1000,
            Tags:
            [
                new ModbusTagConfiguration(
                    TagId:
                        tagId,
                    Name:
                        tagId,
                    Address:
                        0,
                    Writable:
                        false)
            ]);
    }

    private static AlarmDefinitionConfiguration CreateHighDefinition(
        string alarmId,
        string tagId,
        decimal threshold,
        decimal hysteresis,
        int delayMilliseconds = 0)
    {
        return new AlarmDefinitionConfiguration(
            AlarmId:
                alarmId,
            Name:
                alarmId,
            Enabled:
                true,
            TagId:
                tagId,
            Condition:
                AlarmCondition.High,
            Threshold:
                threshold,
            Severity:
                AlarmSeverity.Warning,
            Message:
                "High alarm",
            DelayMilliseconds:
                delayMilliseconds,
            Hysteresis:
                hysteresis);
    }

    private static AlarmDefinitionConfiguration CreateDigitalDefinition(
        string alarmId,
        string tagId,
        AlarmCondition condition)
    {
        return new AlarmDefinitionConfiguration(
            AlarmId:
                alarmId,
            Name:
                alarmId,
            Enabled:
                true,
            TagId:
                tagId,
            Condition:
                condition,
            Threshold:
                null,
            Severity:
                AlarmSeverity.Information,
            Message:
                "Digital alarm",
            DelayMilliseconds:
                0,
            Hysteresis:
                null);
    }

    private static void AssertState(
        AlarmRuntimeService runtime,
        string alarmId,
        AlarmRuntimeState expected)
    {
        var snapshot =
            runtime.Get(
                alarmId)
            ?? throw new InvalidOperationException(
                $"Alarm '{alarmId}' was not found in runtime state.");

        Assert.AreEqual(
            expected,
            snapshot.State);
    }

    private static async Task WaitForStateAsync(
        AlarmRuntimeService runtime,
        string alarmId,
        AlarmRuntimeState expected)
    {
        for (var attempt = 0;
             attempt < 100;
             attempt++)
        {
            if (runtime.Get(
                    alarmId)?.State == expected)
            {
                return;
            }

            await Task.Delay(
                20);
        }

        Assert.Fail(
            $"Alarm '{alarmId}' did not reach state '{expected}'.");
    }

    private sealed class RuntimeEnvironment : IDisposable
    {
        private readonly string _directory;
        private bool _stopped;

        private RuntimeEnvironment(
            string directory,
            TagService tags,
            AlarmDefinitionCatalog definitions,
            ConfigurationCatalog configuration,
            SqliteOperationalStore store,
            EventJournalService eventJournal,
            AlarmRuntimeService runtime)
        {
            _directory =
                directory;
            Tags =
                tags;
            Definitions =
                definitions;
            Configuration =
                configuration;
            Store =
                store;
            EventJournal =
                eventJournal;
            Runtime =
                runtime;
        }

        public TagService Tags { get; }

        public AlarmDefinitionCatalog Definitions { get; }

        public ConfigurationCatalog Configuration { get; }

        public SqliteOperationalStore Store { get; }

        public EventJournalService EventJournal { get; }

        public AlarmRuntimeService Runtime { get; }

        public static async Task<RuntimeEnvironment> CreateAsync(
            IReadOnlyCollection<AlarmDefinitionConfiguration> definitions)
        {
            var directory =
                Path.Combine(
                    Path.GetTempPath(),
                    "dispatcher-alarm-runtime-tests",
                    Guid.NewGuid().ToString(
                        "N"));

            Directory.CreateDirectory(
                directory);

            var tags =
                new TagService();
            var catalog =
                new AlarmDefinitionCatalog();

            catalog.ReplaceAll(
                definitions);

            var configuration =
                new ConfigurationCatalog();
            var store =
                new SqliteOperationalStore(
                    Path.Combine(
                        directory,
                        "operational.db"));
            var eventJournal =
                new EventJournalService(
                    new DeviceStateService(),
                    store,
                    new EventJournalOptions(
                        BufferCapacity:
                            128,
                        BatchSize:
                            16),
                    NullLogger<EventJournalService>.Instance);
            var runtime =
                new AlarmRuntimeService(
                    tags,
                    catalog,
                    configuration,
                    eventJournal,
                    NullLogger<AlarmRuntimeService>.Instance);

            var environment =
                new RuntimeEnvironment(
                    directory,
                    tags,
                    catalog,
                    configuration,
                    store,
                    eventJournal,
                    runtime);

            try
            {
                await eventJournal.StartAsync(
                    CancellationToken.None);
                await runtime.StartAsync(
                    CancellationToken.None);

                return environment;
            }
            catch
            {
                environment.Dispose();
                throw;
            }
        }

        public async Task StopAsync()
        {
            if (_stopped)
            {
                return;
            }

            await Runtime.StopAsync(
                CancellationToken.None);
            await EventJournal.StopAsync(
                CancellationToken.None);

            _stopped =
                true;
        }

        public void Dispose()
        {
            if (!_stopped)
            {
                Runtime.StopAsync(
                        CancellationToken.None)
                    .GetAwaiter()
                    .GetResult();
                EventJournal.StopAsync(
                        CancellationToken.None)
                    .GetAwaiter()
                    .GetResult();
                _stopped =
                    true;
            }

            Runtime.Dispose();

            if (Directory.Exists(
                    _directory))
            {
                Directory.Delete(
                    _directory,
                    recursive:
                        true);
            }
        }
    }
}
