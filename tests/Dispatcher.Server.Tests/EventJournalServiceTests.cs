using Dispatcher.Core.Devices;
using Dispatcher.Server.Events;
using Dispatcher.Server.Historian;
using Microsoft.Extensions.Logging.Abstractions;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class EventJournalServiceTests
{
    [TestMethod]
    public async Task StartStop_WritesSystemLifecycleEvents()
    {
        var directory =
            CreateTempDirectory();

        try
        {
            var store =
                new SqliteOperationalStore(
                    Path.Combine(
                        directory,
                        "operational.db"));

            var service =
                new EventJournalService(
                    new DeviceStateService(),
                    store,
                    new EventJournalOptions(
                        32,
                        8),
                    NullLogger<EventJournalService>.Instance);

            await service.StartAsync(
                CancellationToken.None);

            await service.StopAsync(
                CancellationToken.None);

            var events =
                await store.LoadAllEventsAsync();

            CollectionAssert.AreEqual(
                new[]
                {
                    EventTypes.SystemStarted,
                    EventTypes.SystemStopping
                },
                events
                    .Select(record => record.Type)
                    .ToArray());

            Assert.IsTrue(
                events.All(record =>
                    record.Category == EventCategory.System
                    && record.Source == "server"));
        }
        finally
        {
            Directory.Delete(
                directory,
                recursive: true);
        }
    }

    [TestMethod]
    public async Task DeviceState_RecordsOnlyStatusTransitions()
    {
        var directory =
            CreateTempDirectory();

        try
        {
            var store =
                new SqliteOperationalStore(
                    Path.Combine(
                        directory,
                        "operational.db"));

            var deviceStates =
                new DeviceStateService();

            var service =
                new EventJournalService(
                    deviceStates,
                    store,
                    new EventJournalOptions(
                        32,
                        8),
                    NullLogger<EventJournalService>.Instance);

            await service.StartAsync(
                CancellationToken.None);

            var online1 =
                new DateTimeOffset(
                    2026,
                    8,
                    15,
                    18,
                    0,
                    0,
                    TimeSpan.Zero);

            var onlineAgain =
                online1.AddSeconds(1);

            var offline =
                online1.AddSeconds(2);

            var online2 =
                online1.AddSeconds(3);

            deviceStates.SetOnline(
                "device01",
                online1);

            deviceStates.SetOnline(
                "device01",
                onlineAgain);

            deviceStates.SetOffline(
                "device01",
                "timeout",
                offline);

            deviceStates.SetOffline(
                "device01",
                "timeout again",
                offline.AddMilliseconds(100));

            deviceStates.SetOnline(
                "device01",
                online2);

            await service.StopAsync(
                CancellationToken.None);

            var deviceEvents =
                (await store.LoadAllEventsAsync())
                    .Where(record =>
                        record.Category == EventCategory.Device)
                    .ToArray();

            Assert.AreEqual(
                3,
                deviceEvents.Length);

            CollectionAssert.AreEqual(
                new[]
                {
                    EventTypes.DeviceOnline,
                    EventTypes.DeviceOffline,
                    EventTypes.DeviceOnline
                },
                deviceEvents
                    .Select(record => record.Type)
                    .ToArray());

            CollectionAssert.AreEqual(
                new[]
                {
                    online1,
                    offline,
                    online2
                },
                deviceEvents
                    .Select(record => record.Timestamp)
                    .ToArray());

            Assert.AreEqual(
                EventSeverity.Warning,
                deviceEvents[1].Severity);
            Assert.AreEqual(
                "device01",
                deviceEvents[1].Source);
            StringAssert.Contains(
                deviceEvents[1].DataJson
                ?? string.Empty,
                "timeout");
        }
        finally
        {
            Directory.Delete(
                directory,
                recursive: true);
        }
    }

    [TestMethod]
    public async Task FullBuffer_DropsIncomingEventWithoutBlockingPublisher()
    {
        var store =
            new BlockingEventStore();

        var service =
            new EventJournalService(
                new DeviceStateService(),
                store,
                new EventJournalOptions(
                    BufferCapacity:
                        1,
                    BatchSize:
                        1),
                NullLogger<EventJournalService>.Instance);

        await service.StartAsync(
            CancellationToken.None);

        await store.AppendStarted.Task.WaitAsync(
            TimeSpan.FromSeconds(2));

        var accepted =
            service.Publish(
                EventCategory.System,
                "TestEvent1",
                EventSeverity.Information,
                "test",
                "First buffered event.");

        var dropped =
            service.Publish(
                EventCategory.System,
                "TestEvent2",
                EventSeverity.Information,
                "test",
                "Second buffered event.");

        Assert.IsTrue(
            accepted);
        Assert.IsFalse(
            dropped);
        Assert.IsTrue(
            service.DroppedEventCount >= 1);

        store.Release.TrySetResult(
            true);

        await service.StopAsync(
            CancellationToken.None);
    }

    [TestMethod]
    public async Task PersistedNotification_ContainsStoredEventId()
    {
        var directory =
            CreateTempDirectory();

        try
        {
            var store =
                new SqliteOperationalStore(
                    Path.Combine(
                        directory,
                        "operational.db"));

            var service =
                new EventJournalService(
                    new DeviceStateService(),
                    store,
                    new EventJournalOptions(
                        32,
                        8),
                    NullLogger<EventJournalService>.Instance);

            await service.StartAsync(
                CancellationToken.None);

            var persisted =
                new TaskCompletionSource<EventRecord>(
                    TaskCreationOptions.RunContinuationsAsynchronously);

            service.Persisted +=
                record =>
                {
                    if (string.Equals(
                            record.Type,
                            "PersistedTest",
                            StringComparison.Ordinal))
                    {
                        persisted.TrySetResult(
                            record);
                    }
                };

            var accepted =
                service.Publish(
                    EventCategory.System,
                    "PersistedTest",
                    EventSeverity.Information,
                    "test",
                    "Persisted event.");

            Assert.IsTrue(
                accepted);

            var notified =
                await persisted.Task.WaitAsync(
                    TimeSpan.FromSeconds(2));

            Assert.IsTrue(
                notified.EventId > 0);

            var stored =
                (await store.LoadAllEventsAsync())
                    .Single(record =>
                        record.EventId == notified.EventId);

            Assert.AreEqual(
                "PersistedTest",
                stored.Type);
            Assert.AreEqual(
                notified.Timestamp,
                stored.Timestamp);

            await service.StopAsync(
                CancellationToken.None);
        }
        finally
        {
            Directory.Delete(
                directory,
                recursive: true);
        }
    }

    private static string CreateTempDirectory()
    {
        var directory =
            Path.Combine(
                Path.GetTempPath(),
                "dispatcher-event-tests",
                Guid.NewGuid().ToString(
                    "N"));

        Directory.CreateDirectory(
            directory);

        return directory;
    }

    private sealed class BlockingEventStore : IEventJournalStore
    {
        public TaskCompletionSource<bool> AppendStarted { get; } =
            new(
                TaskCreationOptions.RunContinuationsAsynchronously);

        public TaskCompletionSource<bool> Release { get; } =
            new(
                TaskCreationOptions.RunContinuationsAsynchronously);

        public Task InitializeAsync(
            CancellationToken cancellationToken = default)
        {
            return Task.CompletedTask;
        }

        public async Task<IReadOnlyList<EventRecord>> AppendEventsAsync(
            IReadOnlyList<EventRecord> events,
            CancellationToken cancellationToken = default)
        {
            AppendStarted.TrySetResult(
                true);

            await Release.Task.WaitAsync(
                cancellationToken);

            return events
                .Select((record, index) =>
                    record with
                    {
                        EventId =
                            index + 1,
                        Timestamp =
                            record.Timestamp.ToUniversalTime()
                    })
                .ToArray();
        }

        public Task<IReadOnlyList<EventRecord>> QueryEventsAsync(
            DateTimeOffset from,
            DateTimeOffset to,
            EventCategory? category,
            EventSeverity? severity,
            string? source,
            string? text,
            int offset,
            int limit,
            CancellationToken cancellationToken = default)
        {
            return Task.FromResult<IReadOnlyList<EventRecord>>(
                Array.Empty<EventRecord>());
        }

        public Task<IReadOnlyList<EventRecord>> LoadAllEventsAsync(
            CancellationToken cancellationToken = default)
        {
            return Task.FromResult<IReadOnlyList<EventRecord>>(
                Array.Empty<EventRecord>());
        }
    }
}
