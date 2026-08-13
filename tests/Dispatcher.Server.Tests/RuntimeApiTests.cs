using System.Net;
using System.Net.Http.Json;
using System.Text.Json;
using Dispatcher.Contracts.Devices;
using Dispatcher.Contracts.Tags;
using Dispatcher.Core.Devices;
using Dispatcher.Core.Tags;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class RuntimeApiTests
{
    [TestMethod]
    public async Task GetTags_ReturnsCurrentTagSnapshot()
    {
        using var factory = new WebApplicationFactory<Program>();

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
            (ushort)1234,
            timestamp);

        using var client = factory.CreateClient();

        var response = await client.GetAsync("/api/tags");

        Assert.AreEqual(HttpStatusCode.OK, response.StatusCode);

        var tags = await response.Content
            .ReadFromJsonAsync<TagValueDto[]>();

        Assert.IsNotNull(tags);
        Assert.AreEqual(1, tags.Length);
        Assert.AreEqual("device01.register100", tags[0].TagId);
        Assert.AreEqual(timestamp, tags[0].Timestamp);

        var jsonValue = (JsonElement)tags[0].Value!;
        Assert.AreEqual(1234, jsonValue.GetInt32());
    }

    [TestMethod]
    public async Task GetDevices_ReturnsCurrentDeviceStateSnapshot()
    {
        using var factory = new WebApplicationFactory<Program>();

        var deviceStateService =
            factory.Services.GetRequiredService<DeviceStateService>();

        var onlineAt = new DateTimeOffset(
            2026,
            8,
            13,
            20,
            0,
            0,
            TimeSpan.Zero);
        var offlineAt = onlineAt.AddSeconds(5);

        deviceStateService.SetOnline(
            "device01",
            onlineAt);
        deviceStateService.SetOffline(
            "device01",
            "Connection lost.",
            offlineAt);

        using var client = factory.CreateClient();

        var response = await client.GetAsync("/api/devices");

        Assert.AreEqual(HttpStatusCode.OK, response.StatusCode);

        var devices = await response.Content
            .ReadFromJsonAsync<DeviceStateDto[]>();

        Assert.IsNotNull(devices);
        Assert.AreEqual(1, devices.Length);
        Assert.AreEqual("device01", devices[0].DeviceId);
        Assert.AreEqual(
            DeviceConnectionStatusDto.Offline,
            devices[0].Status);
        Assert.AreEqual(offlineAt, devices[0].UpdatedAt);
        Assert.AreEqual(
            onlineAt,
            devices[0].LastSuccessfulPollAt);
        Assert.AreEqual(
            "Connection lost.",
            devices[0].Error);
    }

    [TestMethod]
    public async Task RuntimeEndpoints_WhenStateIsEmpty_ReturnEmptyArrays()
    {
        using var factory = new WebApplicationFactory<Program>();
        using var client = factory.CreateClient();

        var tags = await client
            .GetFromJsonAsync<TagValueDto[]>("/api/tags");
        var devices = await client
            .GetFromJsonAsync<DeviceStateDto[]>("/api/devices");

        Assert.IsNotNull(tags);
        Assert.IsNotNull(devices);
        Assert.AreEqual(0, tags.Length);
        Assert.AreEqual(0, devices.Length);
    }
}
