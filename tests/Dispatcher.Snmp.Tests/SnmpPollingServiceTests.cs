using System.Net;
using System.Net.Sockets;
using Dispatcher.Core.Devices;
using Dispatcher.Core.Tags;
using Dispatcher.Snmp;
using Dispatcher.Snmp.Configuration;
using Dispatcher.Tests.Shared;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Snmp.Tests;

[TestClass]
public sealed class SnmpPollingServiceTests
{
    [TestMethod]
    public async Task PollOnceAsync_Integer32Response_PublishesTagAndOnline()
    {
        using var agent =
            new SnmpV2cTestAgent(
                "public",
                "1.3.6.1.2.1.1.7.0");

        var serverTask =
            agent.ServeInteger32OnceAsync(
                2468);

        var tagService =
            new TagService();
        var deviceStateService =
            new DeviceStateService();

        var polling =
            new SnmpPollingService(
                tagService,
                deviceStateService,
                new SnmpGetClient());

        var state =
            await polling.PollOnceAsync(
                CreatePlan(
                    agent.Port,
                    "1.3.6.1.2.1.1.7.0",
                    "snmp01.sysServices"));

        await serverTask.WaitAsync(
            TimeSpan.FromSeconds(2));

        Assert.AreEqual(
            DeviceConnectionStatus.Online,
            state.Status);
        Assert.AreEqual(
            2468,
            tagService.Get(
                "snmp01.sysServices")?.Value);
    }

    [TestMethod]
    public async Task PollOnceAsync_OctetStringResponse_PublishesString()
    {
        using var agent =
            new SnmpV2cTestAgent(
                "public",
                "1.3.6.1.2.1.1.5.0");

        var serverTask =
            agent.ServeOctetStringOnceAsync(
                "dispatcher-agent");

        var tagService =
            new TagService();

        var polling =
            new SnmpPollingService(
                tagService,
                new DeviceStateService(),
                new SnmpGetClient());

        var state =
            await polling.PollOnceAsync(
                CreatePlan(
                    agent.Port,
                    "1.3.6.1.2.1.1.5.0",
                    "snmp01.sysName"));

        await serverTask.WaitAsync(
            TimeSpan.FromSeconds(2));

        Assert.AreEqual(
            DeviceConnectionStatus.Online,
            state.Status);
        Assert.AreEqual(
            "dispatcher-agent",
            tagService.Get(
                "snmp01.sysName")?.Value);
    }

    [TestMethod]
    public async Task PollOnceAsync_NoResponse_MarksDeviceOffline()
    {
        var port =
            ReserveUnusedUdpPort();

        var polling =
            new SnmpPollingService(
                new TagService(),
                new DeviceStateService(),
                new SnmpGetClient());

        var plan =
            CreatePlan(
                port,
                "1.3.6.1.2.1.1.5.0",
                "snmp01.sysName",
                requestTimeout:
                    TimeSpan.FromMilliseconds(150));

        var state =
            await polling.PollOnceAsync(
                plan);

        Assert.AreEqual(
            DeviceConnectionStatus.Offline,
            state.Status);
        Assert.IsFalse(
            string.IsNullOrWhiteSpace(
                state.Error));
    }

    private static SnmpPollingPlan CreatePlan(
        int port,
        string oid,
        string tagId,
        TimeSpan? requestTimeout = null)
    {
        return new SnmpPollingPlan(
            new SnmpV2cDevice(
                "snmp01",
                IPAddress.Loopback.ToString(),
                port,
                "public"),
            [
                new SnmpPoint(
                    tagId,
                    oid)
            ],
            TimeSpan.FromSeconds(10),
            requestTimeout
                ?? TimeSpan.FromSeconds(1));
    }

    private static int ReserveUnusedUdpPort()
    {
        using var udp =
            new UdpClient(
                new IPEndPoint(
                    IPAddress.Loopback,
                    0));

        return ((IPEndPoint)udp.Client.LocalEndPoint!).Port;
    }
}
