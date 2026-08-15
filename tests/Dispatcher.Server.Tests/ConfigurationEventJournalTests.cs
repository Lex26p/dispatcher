using System.Net;
using System.Net.Http.Json;
using Dispatcher.Contracts.Configuration;
using Dispatcher.Server.Events;
using Dispatcher.Server.Historian;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class ConfigurationEventJournalTests
{
    [TestMethod]
    public async Task DeviceConfigurationMutation_AfterRuntimeApply_WritesConfigurationEvent()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        var response =
            await client.PostAsJsonAsync(
                "/api/configuration/modbus/devices",
                new ModbusDeviceUpsertRequest(
                    DeviceId:
                        "event-device",
                    Name:
                        "Event device",
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
                        1000));

        Assert.AreEqual(
            HttpStatusCode.Created,
            response.StatusCode);

        var store =
            new SqliteOperationalStore(
                TestDispatcherFactory.GetOperationalDatabasePath(
                    database.DatabasePath));

        await store.InitializeAsync();

        var record =
            await WaitForEventAsync(
                store,
                EventTypes.RuntimeConfigurationApplied,
                TimeSpan.FromSeconds(2));

        Assert.AreEqual(
            EventCategory.Configuration,
            record.Category);
        Assert.AreEqual(
            EventSeverity.Information,
            record.Severity);
        Assert.AreEqual(
            "configuration",
            record.Source);

        StringAssert.Contains(
            record.DataJson
            ?? string.Empty,
            "\"ModbusDevices\":1");
    }

    private static async Task<EventRecord> WaitForEventAsync(
        SqliteOperationalStore store,
        string eventType,
        TimeSpan timeout)
    {
        var deadline =
            DateTimeOffset.UtcNow
            + timeout;

        while (true)
        {
            var record =
                (await store.LoadAllEventsAsync())
                    .LastOrDefault(current =>
                        string.Equals(
                            current.Type,
                            eventType,
                            StringComparison.Ordinal));

            if (record is not null)
            {
                return record;
            }

            if (DateTimeOffset.UtcNow >= deadline)
            {
                Assert.Fail(
                    $"Event '{eventType}' was not persisted before timeout.");
            }

            await Task.Delay(10);
        }
    }
}
