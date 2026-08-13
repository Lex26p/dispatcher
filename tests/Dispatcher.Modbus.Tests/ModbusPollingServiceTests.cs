using System.Net;
using System.Net.Sockets;
using Dispatcher.Core.Devices;
using Dispatcher.Core.Tags;
using Dispatcher.Modbus.Configuration;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Modbus.Tests;

[TestClass]
public sealed class ModbusPollingServiceTests
{
    [TestMethod]
    public async Task PollOnceAsync_ReadsMultiplePoints_AndMarksDeviceOnline()
    {
        using var server = new TestModbusTcpServer(
            expectedUnitId: 1,
            valueFactory: (_, _, address) => address switch
            {
                100 => 1234,
                101 => 5678,
                _ => throw new InvalidDataException(
                    $"Unexpected address {address}.")
            });

        var serverTask = server.ServeAsync(2);

        var tagService = new TagService();
        var deviceStateService = new DeviceStateService();
        var pollingService = CreatePollingService(
            tagService,
            deviceStateService);

        var plan = CreatePlan(
            server.Port,
            new ModbusHoldingRegisterPoint(
                "device01.register100",
                100),
            new ModbusHoldingRegisterPoint(
                "device01.register101",
                101));

        var state = await pollingService.PollOnceAsync(plan);
        await serverTask;

        Assert.AreEqual(DeviceConnectionStatus.Online, state.Status);
        Assert.IsNotNull(state.LastSuccessfulPollAt);
        Assert.IsNull(state.Error);

        Assert.AreEqual(
            (ushort)1234,
            tagService.Get("device01.register100")?.Value);
        Assert.AreEqual(
            (ushort)5678,
            tagService.Get("device01.register101")?.Value);
    }

    [TestMethod]
    public async Task PollOnceAsync_WhenConnectionFails_MarksDeviceOffline()
    {
        var listener = new TcpListener(IPAddress.Loopback, 0);
        listener.Start();

        var port = ((IPEndPoint)listener.LocalEndpoint).Port;
        listener.Stop();

        var tagService = new TagService();
        var deviceStateService = new DeviceStateService();
        var pollingService = CreatePollingService(
            tagService,
            deviceStateService);

        var plan = CreatePlan(
            port,
            new ModbusHoldingRegisterPoint(
                "device01.register100",
                100));

        var state = await pollingService.PollOnceAsync(plan);

        Assert.AreEqual(DeviceConnectionStatus.Offline, state.Status);
        Assert.IsFalse(string.IsNullOrWhiteSpace(state.Error));
        Assert.IsNull(tagService.Get("device01.register100"));
    }


    [TestMethod]
    public async Task PollOnceAsync_WhenResponseTimesOut_MarksDeviceOffline()
    {
        using var server = new TestModbusTcpServer(
            expectedUnitId: 1,
            valueFactory: (_, _, _) => 0);

        var serverTask = server.AcceptAndIgnoreOneRequestAsync(
            TimeSpan.FromMilliseconds(250));

        var tagService = new TagService();
        var deviceStateService = new DeviceStateService();
        var pollingService = CreatePollingService(
            tagService,
            deviceStateService);

        var plan = new ModbusPollingPlan(
            Device: new ModbusTcpDevice(
                DeviceId: "device01",
                Host: IPAddress.Loopback.ToString(),
                Port: server.Port,
                UnitId: 1),
            Points:
            [
                new ModbusHoldingRegisterPoint(
                    "device01.register100",
                    100)
            ],
            PollInterval: TimeSpan.FromMilliseconds(100),
            RequestTimeout: TimeSpan.FromMilliseconds(50));

        var state = await pollingService.PollOnceAsync(plan);
        await serverTask;

        Assert.AreEqual(DeviceConnectionStatus.Offline, state.Status);
        StringAssert.Contains(state.Error ?? string.Empty, "Timed out");
        Assert.IsNull(tagService.Get("device01.register100"));
    }

    [TestMethod]
    public async Task RunAsync_ReconnectsAndPollsAgain()
    {
        using var server = new TestModbusTcpServer(
            expectedUnitId: 1,
            valueFactory: (connectionIndex, _, address) =>
            {
                Assert.AreEqual((ushort)100, address);

                return connectionIndex switch
                {
                    0 => 100,
                    1 => 200,
                    _ => throw new InvalidDataException(
                        $"Unexpected connection index {connectionIndex}.")
                };
            });

        var serverTask = server.ServeAsync(1, 1);

        var tagService = new TagService();
        var deviceStateService = new DeviceStateService();
        var pollingService = CreatePollingService(
            tagService,
            deviceStateService);

        var plan = new ModbusPollingPlan(
            Device: new ModbusTcpDevice(
                DeviceId: "device01",
                Host: IPAddress.Loopback.ToString(),
                Port: server.Port,
                UnitId: 1),
            Points:
            [
                new ModbusHoldingRegisterPoint(
                    "device01.register100",
                    100)
            ],
            PollInterval: TimeSpan.FromMilliseconds(20),
            RequestTimeout: TimeSpan.FromSeconds(1));

        using var cancellation = new CancellationTokenSource(
            TimeSpan.FromSeconds(3));

        var pollingTask = pollingService.RunAsync(
            plan,
            cancellation.Token);

        await serverTask.WaitAsync(TimeSpan.FromSeconds(2));

        await WaitUntilAsync(
            () => Equals(
                tagService.Get("device01.register100")?.Value,
                (ushort)200),
            TimeSpan.FromSeconds(1));

        cancellation.Cancel();
        await pollingTask;

        Assert.AreEqual(
            (ushort)200,
            tagService.Get("device01.register100")?.Value);

        var state = deviceStateService.Get("device01");
        Assert.IsNotNull(state);
        Assert.AreEqual(
            DeviceConnectionStatus.Online,
            state.Status);
    }

    private static ModbusPollingService CreatePollingService(
        TagService tagService,
        DeviceStateService deviceStateService)
    {
        return new ModbusPollingService(
            tagService,
            deviceStateService,
            new ModbusTcpRegisterReader());
    }

    private static ModbusPollingPlan CreatePlan(
        int port,
        params ModbusHoldingRegisterPoint[] points)
    {
        return new ModbusPollingPlan(
            Device: new ModbusTcpDevice(
                DeviceId: "device01",
                Host: IPAddress.Loopback.ToString(),
                Port: port,
                UnitId: 1),
            Points: points,
            PollInterval: TimeSpan.FromMilliseconds(100),
            RequestTimeout: TimeSpan.FromSeconds(1));
    }

    private static async Task WaitUntilAsync(
        Func<bool> condition,
        TimeSpan timeout)
    {
        var deadline = DateTimeOffset.UtcNow + timeout;

        while (!condition())
        {
            if (DateTimeOffset.UtcNow >= deadline)
            {
                Assert.Fail(
                    "Condition was not reached before the timeout.");
            }

            await Task.Delay(10);
        }
    }
}
