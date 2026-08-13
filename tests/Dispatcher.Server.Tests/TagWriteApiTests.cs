using System.Net;
using System.Net.Http.Json;
using System.Net.Sockets;
using System.Text.Json;
using Dispatcher.Contracts.Tags;
using Dispatcher.Core.Tags;
using Microsoft.AspNetCore.Hosting;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class TagWriteApiTests
{
    [TestMethod]
    public async Task WriteTag_WritablePoint_SendsFc06_AndUpdatesRuntimeValue()
    {
        using var modbusServer = new ReadThenWriteModbusTcpServer(
            expectedUnitId: 1,
            expectedAddress: 100,
            initialValue: 1234,
            expectedWriteValue: 3456);

        var serverTask = modbusServer.ServeAsync();

        using var factory = CreateFactory(
            modbusServer.Port,
            writable: true);

        using var client = factory.CreateClient();

        var tagService =
            factory.Services.GetRequiredService<TagService>();

        await WaitUntilAsync(
            () => Equals(
                tagService.Get("device01.register100")?.Value,
                (ushort)1234),
            TimeSpan.FromSeconds(2));

        var snapshot = await client.GetFromJsonAsync<TagValueDto[]>(
            "/api/tags");

        Assert.IsNotNull(snapshot);
        Assert.AreEqual(1, snapshot.Length);
        Assert.IsTrue(snapshot[0].Writable);

        var response = await client.PostAsJsonAsync(
            "/api/tags/device01.register100/write",
            new TagWriteRequest(3456));

        Assert.AreEqual(
            HttpStatusCode.OK,
            response.StatusCode);

        var updated = await response.Content
            .ReadFromJsonAsync<TagValueDto>();

        Assert.IsNotNull(updated);
        Assert.AreEqual(
            "device01.register100",
            updated.TagId);
        Assert.IsTrue(updated.Writable);

        var jsonValue = (JsonElement)updated.Value!;
        Assert.AreEqual(3456, jsonValue.GetInt32());

        await serverTask.WaitAsync(
            TimeSpan.FromSeconds(2));

        Assert.AreEqual(
            (ushort)3456,
            modbusServer.WrittenValue);
        Assert.AreEqual(
            (ushort)3456,
            tagService.Get("device01.register100")?.Value);
    }

    [TestMethod]
    public async Task WriteTag_ReadOnlyPoint_ReturnsConflict()
    {
        var port = ReserveUnusedPort();

        using var factory = CreateFactory(
            port,
            writable: false);

        using var client = factory.CreateClient();

        var response = await client.PostAsJsonAsync(
            "/api/tags/device01.register100/write",
            new TagWriteRequest(100));

        Assert.AreEqual(
            HttpStatusCode.Conflict,
            response.StatusCode);
    }

    [TestMethod]
    public async Task WriteTag_ValueOutsideUInt16_ReturnsBadRequest()
    {
        var port = ReserveUnusedPort();

        using var factory = CreateFactory(
            port,
            writable: true);

        using var client = factory.CreateClient();

        var response = await client.PostAsJsonAsync(
            "/api/tags/device01.register100/write",
            new TagWriteRequest(70000));

        Assert.AreEqual(
            HttpStatusCode.BadRequest,
            response.StatusCode);
    }

    private static WebApplicationFactory<Program> CreateFactory(
        int port,
        bool writable)
    {
        return new WebApplicationFactory<Program>()
            .WithWebHostBuilder(builder =>
            {
                builder.ConfigureAppConfiguration((_, configuration) =>
                {
                    configuration.AddInMemoryCollection(
                        CreateModbusConfiguration(
                            port,
                            writable));
                });
            });
    }

    private static Dictionary<string, string?> CreateModbusConfiguration(
        int port,
        bool writable)
    {
        return new Dictionary<string, string?>
        {
            ["Modbus:Enabled"] = "true",
            ["Modbus:Device:DeviceId"] = "device01",
            ["Modbus:Device:Host"] =
                IPAddress.Loopback.ToString(),
            ["Modbus:Device:Port"] = port.ToString(),
            ["Modbus:Device:UnitId"] = "1",
            ["Modbus:Device:PollIntervalMilliseconds"] = "10000",
            ["Modbus:Device:RequestTimeoutMilliseconds"] = "1000",
            ["Modbus:Device:Points:0:TagId"] =
                "device01.register100",
            ["Modbus:Device:Points:0:Address"] = "100",
            ["Modbus:Device:Points:0:Writable"] =
                writable.ToString()
        };
    }

    private static int ReserveUnusedPort()
    {
        var listener = new TcpListener(
            IPAddress.Loopback,
            0);

        listener.Start();
        var port =
            ((IPEndPoint)listener.LocalEndpoint).Port;
        listener.Stop();

        return port;
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
                    "Expected runtime value was not published before timeout.");
            }

            await Task.Delay(10);
        }
    }

    private sealed class ReadThenWriteModbusTcpServer : IDisposable
    {
        private readonly TcpListener _listener;
        private readonly byte _expectedUnitId;
        private readonly ushort _expectedAddress;
        private readonly ushort _initialValue;
        private readonly ushort _expectedWriteValue;

        public ReadThenWriteModbusTcpServer(
            byte expectedUnitId,
            ushort expectedAddress,
            ushort initialValue,
            ushort expectedWriteValue)
        {
            _expectedUnitId = expectedUnitId;
            _expectedAddress = expectedAddress;
            _initialValue = initialValue;
            _expectedWriteValue = expectedWriteValue;

            _listener = new TcpListener(
                IPAddress.Loopback,
                0);
            _listener.Start();

            Port =
                ((IPEndPoint)_listener.LocalEndpoint).Port;
        }

        public int Port { get; }

        public ushort? WrittenValue { get; private set; }

        public async Task ServeAsync()
        {
            await ServeReadAsync();
            await ServeWriteAsync();
        }

        private async Task ServeReadAsync()
        {
            using var client =
                await _listener.AcceptTcpClientAsync();
            using var stream = client.GetStream();

            var request = new byte[12];
            await ReadExactlyAsync(
                stream,
                request);

            ValidateHeader(
                request,
                expectedFunctionCode: 3);

            var quantity =
                (ushort)((request[10] << 8) | request[11]);

            Assert.AreEqual(
                (ushort)1,
                quantity);

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
                (byte)(_initialValue >> 8),
                (byte)_initialValue
            };

            await stream.WriteAsync(response);
        }

        private async Task ServeWriteAsync()
        {
            using var client =
                await _listener.AcceptTcpClientAsync();
            using var stream = client.GetStream();

            var request = new byte[12];
            await ReadExactlyAsync(
                stream,
                request);

            ValidateHeader(
                request,
                expectedFunctionCode: 6);

            var value =
                (ushort)((request[10] << 8) | request[11]);

            Assert.AreEqual(
                _expectedWriteValue,
                value);

            WrittenValue = value;

            await stream.WriteAsync(request);
        }

        private void ValidateHeader(
            byte[] request,
            byte expectedFunctionCode)
        {
            var unitId = request[6];
            var functionCode = request[7];
            var address =
                (ushort)((request[8] << 8) | request[9]);

            Assert.AreEqual(
                _expectedUnitId,
                unitId);
            Assert.AreEqual(
                expectedFunctionCode,
                functionCode);
            Assert.AreEqual(
                _expectedAddress,
                address);
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
