using Dispatcher.Core.Devices;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Core.Tests;

[TestClass]
public sealed class DeviceStateServiceTests
{
    [TestMethod]
    public void SetOnline_StoresSuccessfulPollTimestamp()
    {
        var service = new DeviceStateService();
        var timestamp = new DateTimeOffset(
            2026,
            8,
            13,
            20,
            0,
            0,
            TimeSpan.Zero);

        var state = service.SetOnline("device01", timestamp);

        Assert.AreEqual(DeviceConnectionStatus.Online, state.Status);
        Assert.AreEqual(timestamp, state.UpdatedAt);
        Assert.AreEqual(timestamp, state.LastSuccessfulPollAt);
        Assert.IsNull(state.Error);
        Assert.AreEqual(state, service.Get("device01"));
    }

    [TestMethod]
    public void SetOffline_PreservesLastSuccessfulPollTimestamp()
    {
        var service = new DeviceStateService();
        var onlineTimestamp = new DateTimeOffset(
            2026,
            8,
            13,
            20,
            0,
            0,
            TimeSpan.Zero);
        var offlineTimestamp = onlineTimestamp.AddSeconds(1);

        service.SetOnline("device01", onlineTimestamp);

        var state = service.SetOffline(
            "device01",
            "Connection lost.",
            offlineTimestamp);

        Assert.AreEqual(DeviceConnectionStatus.Offline, state.Status);
        Assert.AreEqual(offlineTimestamp, state.UpdatedAt);
        Assert.AreEqual(onlineTimestamp, state.LastSuccessfulPollAt);
        Assert.AreEqual("Connection lost.", state.Error);
    }

    [TestMethod]
    public void GetAll_ReturnsStatesOrderedByDeviceId()
    {
        var service = new DeviceStateService();
        var timestamp = DateTimeOffset.UtcNow;

        service.SetOnline("device02", timestamp);
        service.SetOnline("device01", timestamp);

        var states = service.GetAll();

        Assert.AreEqual(2, states.Count);
        Assert.AreEqual("device01", states[0].DeviceId);
        Assert.AreEqual("device02", states[1].DeviceId);
    }
}
