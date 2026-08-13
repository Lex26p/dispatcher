using System.Text.Json;
using Dispatcher.Contracts.Devices;
using Dispatcher.Contracts.Realtime;
using Dispatcher.Contracts.Tags;
using Dispatcher.Core.Devices;
using Dispatcher.Core.Tags;
using Microsoft.AspNetCore.Http.Connections;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.AspNetCore.SignalR.Client;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class RuntimeHubTests
{
    [TestMethod]
    public async Task TagChange_IsPublishedThroughSignalR()
    {
        using var factory = new WebApplicationFactory<Program>();
        await using var connection = CreateConnection(factory);

        var received = new TaskCompletionSource<TagValueDto>(
            TaskCreationOptions.RunContinuationsAsynchronously);

        connection.On<TagValueDto>(
            RuntimeHubContract.TagChanged,
            value => received.TrySetResult(value));

        await connection.StartAsync();

        var tagService = factory.Services.GetRequiredService<TagService>();
        var timestamp = new DateTimeOffset(
            2026,
            8,
            13,
            20,
            0,
            0,
            TimeSpan.Zero);

        tagService.Set(
            "device01.register100",
            (ushort)4321,
            timestamp);

        var update = await received.Task.WaitAsync(
            TimeSpan.FromSeconds(2));

        Assert.AreEqual("device01.register100", update.TagId);
        Assert.AreEqual(timestamp, update.Timestamp);

        var jsonValue = (JsonElement)update.Value!;
        Assert.AreEqual(4321, jsonValue.GetInt32());
    }

    [TestMethod]
    public async Task DeviceStateChange_IsPublishedThroughSignalR()
    {
        using var factory = new WebApplicationFactory<Program>();
        await using var connection = CreateConnection(factory);

        var received = new TaskCompletionSource<DeviceStateDto>(
            TaskCreationOptions.RunContinuationsAsynchronously);

        connection.On<DeviceStateDto>(
            RuntimeHubContract.DeviceStateChanged,
            value => received.TrySetResult(value));

        await connection.StartAsync();

        var deviceStateService =
            factory.Services.GetRequiredService<DeviceStateService>();

        var timestamp = new DateTimeOffset(
            2026,
            8,
            13,
            20,
            0,
            0,
            TimeSpan.Zero);

        deviceStateService.SetOnline(
            "device01",
            timestamp);

        var update = await received.Task.WaitAsync(
            TimeSpan.FromSeconds(2));

        Assert.AreEqual("device01", update.DeviceId);
        Assert.AreEqual(
            DeviceConnectionStatusDto.Online,
            update.Status);
        Assert.AreEqual(timestamp, update.UpdatedAt);
        Assert.AreEqual(
            timestamp,
            update.LastSuccessfulPollAt);
        Assert.IsNull(update.Error);
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
