using Dispatcher.Contracts.Alarms;
using Dispatcher.Contracts.Realtime;
using Dispatcher.Core.Tags;
using Dispatcher.Server.Alarms;
using Dispatcher.Server.Configuration;
using Microsoft.AspNetCore.Http.Connections;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.AspNetCore.SignalR.Client;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class AlarmRealtimeTests
{
    [TestMethod]
    public async Task AlarmTransition_IsPublishedThroughSignalR()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync(
                TestModbusConfiguration.CreateDevice(
                    port:
                        502,
                    enabled:
                        false));

        var store =
            new SqliteConfigurationStore(
                database.DatabasePath);

        await store.InsertAlarmDefinitionAsync(
            new AlarmDefinitionConfiguration(
                AlarmId:
                    "realtime.high",
                Name:
                    "Realtime high",
                Enabled:
                    true,
                TagId:
                    "device01.register100",
                Condition:
                    AlarmCondition.High,
                Threshold:
                    10m,
                Severity:
                    AlarmSeverity.Error,
                Message:
                    "Realtime alarm.",
                DelayMilliseconds:
                    0,
                Hysteresis:
                    1m));

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);
        await using var connection =
            CreateConnection(
                factory);

        var received =
            new TaskCompletionSource<AlarmRuntimeSnapshotDto>(
                TaskCreationOptions.RunContinuationsAsynchronously);

        connection.On<AlarmRuntimeSnapshotDto>(
            RuntimeHubContract.AlarmChanged,
            snapshot =>
            {
                if (snapshot.AlarmId == "realtime.high"
                    && snapshot.State
                    == AlarmRuntimeStateDto.ActiveUnacknowledged)
                {
                    received.TrySetResult(
                        snapshot);
                }
            });

        await connection.StartAsync();

        var tagService =
            factory.Services.GetRequiredService<TagService>();
        var timestamp =
            new DateTimeOffset(
                2026,
                8,
                16,
                18,
                30,
                0,
                TimeSpan.Zero);

        tagService.Set(
            "device01.register100",
            20,
            timestamp);

        var snapshot =
            await received.Task.WaitAsync(
                TimeSpan.FromSeconds(2));

        Assert.AreEqual(
            "realtime.high",
            snapshot.AlarmId);
        Assert.AreEqual(
            AlarmSeverityDto.Error,
            snapshot.Severity);
        Assert.AreEqual(
            timestamp,
            snapshot.RaisedAt);
        Assert.IsNotNull(
            snapshot.CurrentValue);
    }

    private static HubConnection CreateConnection(
        WebApplicationFactory<Program> factory)
    {
        return new HubConnectionBuilder()
            .WithUrl(
                new Uri(
                    factory.Server.BaseAddress,
                    RuntimeHubContract.Path),
                options =>
                {
                    options.Transports =
                        HttpTransportType.LongPolling;

                    options.HttpMessageHandlerFactory =
                        _ => factory.Server.CreateHandler();
                })
            .Build();
    }
}
