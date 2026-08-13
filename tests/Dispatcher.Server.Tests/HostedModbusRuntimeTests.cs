using System.Net;
using System.Net.Http.Json;
using System.Net.Sockets;
using Dispatcher.Contracts.Devices;
using Dispatcher.Contracts.Tags;
using Dispatcher.Core.Devices;
using Dispatcher.Core.Tags;
using Microsoft.AspNetCore.Hosting;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class HostedModbusRuntimeTests
{
    [TestMethod]
    public async Task EnabledModbusRuntime_PollsConfiguredPoint_AndPublishesRuntimeState()
    {
        using var modbusServer = new SingleReadModbusTcpServer(
            expectedUnitId: 1,
            expectedAddress: 100,
            registerValue: 2468);

        var serverTask = modbusServer.ServeOnceAsync();

        using var factory = new WebApplicationFactory<Program>()
            .WithWebHostBuilder(builder =>
            {
                builder.ConfigureAppConfiguration((_, configuration) =>
                {
                    configuration.AddInMemoryCollection(
                        CreateModbusConfiguration(
                            modbusServer.Port));
                });
            });

        using var client = factory.CreateClient();

        await serverTask.WaitAsync(TimeSpan.FromSeconds(2));

        var tagService =
            factory.Services.GetRequiredService<TagService>();
        var deviceStateService =
            factory.Services.GetRequiredService<DeviceStateService>();

        await WaitUntilAsync(
            () =>
                Equals(
                    tagService.Get("device01.register100")?.Value,
                    (ushort)2468)
                && deviceStateService.Get("device01")?.Status
                    == DeviceConnectionStatus.Online,
            TimeSpan.FromSeconds(2));

        var tags = await client.GetFromJsonAsync<TagValueDto[]>(
            "/api/tags");
        var devices = await client.GetFromJsonAsync<DeviceStateDto[]>(
            "/api/devices");

        Assert.IsNotNull(tags);
        Assert.IsNotNull(devices);
        Assert.AreEqual(1, tags.Length);
        Assert.AreEqual(1, devices.Length);

        Assert.AreEqual(
            "device01.register100",
            tags[0].TagId);

        var tagValue = (System.Text.Json.JsonElement)tags[0].Value!;
        Assert.AreEqual(2468, tagValue.GetInt32());

        Assert.AreEqual("device01", devices[0].DeviceId);
        Assert.AreEqual(
            DeviceConnectionStatusDto.Online,
            devices[0].Status);
        Assert.IsNull(devices[0].Error);
    }

    private static Dictionary<string, string?> CreateModbusConfiguration(
        int port)
    {
        return new Dictionary<string, string?>
        {
            ["Modbus:Enabled"] = "true",
            ["Modbus:Device:DeviceId"] = "device01",
            ["Modbus:Device:Host"] = IPAddress.Loopback.ToString(),
            ["Modbus:Device:Port"] = port.ToString(),
            ["Modbus:Device:UnitId"] = "1",
            ["Modbus:Device:PollIntervalMilliseconds"] = "10000",
            ["Modbus:Device:RequestTimeoutMilliseconds"] = "1000",
            ["Modbus:Device:Points:0:TagId"] =
                "device01.register100",
            ["Modbus:Device:Points:0:Address"] = "100"
        };
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
                    "Hosted Modbus runtime did not publish the expected state before timeout.");
            }

            await Task.Delay(10);
        }
    }

    private sealed class SingleReadModbusTcpServer : IDisposable
    {
        private readonly TcpListener _listener;
        private readonly byte _expectedUnitId;
        private readonly ushort _expectedAddress;
        private readonly ushort _registerValue;

        public SingleReadModbusTcpServer(
            byte expectedUnitId,
            ushort expectedAddress,
            ushort registerValue)
        {
            _expectedUnitId = expectedUnitId;
            _expectedAddress = expectedAddress;
            _registerValue = registerValue;

            _listener = new TcpListener(
                IPAddress.Loopback,
                0);
            _listener.Start();

            Port = ((IPEndPoint)_listener.LocalEndpoint).Port;
        }

        public int Port { get; }

        public async Task ServeOnceAsync()
        {
            using var client =
                await _listener.AcceptTcpClientAsync();
            using var stream = client.GetStream();

            var request = new byte[12];
            await ReadExactlyAsync(stream, request);

            ValidateRequest(request);

            var response = new byte[]
            {
                request[0],
                request[1],
                0,
                0,
                0,
                5,
                _expectedUnitId,
                3,
                2,
                (byte)(_registerValue >> 8),
                (byte)_registerValue
            };

            await stream.WriteAsync(response);
        }

        private void ValidateRequest(byte[] request)
        {
            var unitId = request[6];
            var functionCode = request[7];
            var address =
                (ushort)((request[8] << 8) | request[9]);
            var quantity =
                (ushort)((request[10] << 8) | request[11]);

            Assert.AreEqual(_expectedUnitId, unitId);
            Assert.AreEqual((byte)3, functionCode);
            Assert.AreEqual(_expectedAddress, address);
            Assert.AreEqual((ushort)1, quantity);
        }

        private static async Task ReadExactlyAsync(
            NetworkStream stream,
            byte[] buffer)
        {
            var offset = 0;

            while (offset < buffer.Length)
            {
                var read = await stream.ReadAsync(
                    buffer.AsMemory(
                        offset,
                        buffer.Length - offset));

                if (read == 0)
                {
                    throw new EndOfStreamException(
                        "Modbus TCP client closed the connection before the request was complete.");
                }

                offset += read;
            }
        }

        public void Dispose()
        {
            _listener.Stop();
        }
    }
}
