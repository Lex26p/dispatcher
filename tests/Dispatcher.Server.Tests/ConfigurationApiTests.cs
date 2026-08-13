using System.Net;
using System.Net.Http.Json;
using System.Net.Sockets;
using Dispatcher.Contracts.Configuration;
using Dispatcher.Contracts.Realtime;
using Dispatcher.Core.Tags;
using Dispatcher.Server.Configuration;
using Microsoft.AspNetCore.Http.Connections;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.AspNetCore.SignalR.Client;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class ConfigurationApiTests
{
    [TestMethod]
    public async Task ConfigurationCrud_PersistsDeviceAndTagChanges()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();
        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);
        using var client =
            factory.CreateClient();

        var createDevice =
            new ModbusDeviceUpsertRequest(
                DeviceId: "plc01",
                Name: "PLC 01",
                Enabled: false,
                Host: "192.168.1.10",
                Port: 502,
                UnitId: 1,
                PollIntervalMilliseconds: 1000,
                RequestTimeoutMilliseconds: 1000);

        var createDeviceResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/modbus/devices",
                createDevice);

        Assert.AreEqual(
            HttpStatusCode.Created,
            createDeviceResponse.StatusCode);

        var createTag =
            new ModbusTagUpsertRequest(
                TagId: "plc01.setpoint",
                Name: "Setpoint",
                Address: 100,
                Writable: false);

        var createTagResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/modbus/devices/plc01/tags",
                createTag);

        Assert.AreEqual(
            HttpStatusCode.Created,
            createTagResponse.StatusCode);

        var updateDevice =
            createDevice with
            {
                Name = "Main PLC",
                Port = 1502
            };

        var updateDeviceResponse =
            await client.PutAsJsonAsync(
                "/api/configuration/modbus/devices/plc01",
                updateDevice);

        Assert.AreEqual(
            HttpStatusCode.OK,
            updateDeviceResponse.StatusCode);

        var updateTag =
            createTag with
            {
                Name = "Pressure setpoint",
                Address = 101,
                Writable = true
            };

        var updateTagResponse =
            await client.PutAsJsonAsync(
                "/api/configuration/modbus/devices/plc01/tags/plc01.setpoint",
                updateTag);

        Assert.AreEqual(
            HttpStatusCode.OK,
            updateTagResponse.StatusCode);

        var devices =
            await client.GetFromJsonAsync<
                ModbusDeviceConfigurationDto[]>(
                "/api/configuration/modbus/devices");

        Assert.IsNotNull(devices);
        Assert.AreEqual(1, devices.Length);
        Assert.AreEqual(
            "Main PLC",
            devices[0].Name);
        Assert.AreEqual(
            1502,
            devices[0].Port);
        Assert.AreEqual(
            1,
            devices[0].Tags.Count);
        Assert.AreEqual(
            101,
            devices[0].Tags[0].Address);
        Assert.IsTrue(
            devices[0].Tags[0].Writable);

        var reopenedStore =
            new SqliteConfigurationStore(
                database.DatabasePath);

        await reopenedStore.InitializeAsync();

        var persisted =
            await reopenedStore.LoadAsync();

        Assert.AreEqual(1, persisted.Count);
        Assert.AreEqual(
            "Main PLC",
            persisted[0].Name);
        Assert.AreEqual(
            101,
            persisted[0].Tags.Single().Address);
        Assert.IsTrue(
            persisted[0].Tags.Single().Writable);

        var deleteTagResponse =
            await client.DeleteAsync(
                "/api/configuration/modbus/devices/plc01/tags/plc01.setpoint");

        Assert.AreEqual(
            HttpStatusCode.NoContent,
            deleteTagResponse.StatusCode);

        var deleteDeviceResponse =
            await client.DeleteAsync(
                "/api/configuration/modbus/devices/plc01");

        Assert.AreEqual(
            HttpStatusCode.NoContent,
            deleteDeviceResponse.StatusCode);

        var empty =
            await client.GetFromJsonAsync<
                ModbusDeviceConfigurationDto[]>(
                "/api/configuration/modbus/devices");

        Assert.IsNotNull(empty);
        Assert.AreEqual(0, empty.Length);

        var finalPersisted =
            await reopenedStore.LoadAsync();

        Assert.AreEqual(
            0,
            finalPersisted.Count);
    }

    [TestMethod]
    public async Task ConfigurationCrud_DuplicateIds_ReturnConflict()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();
        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);
        using var client =
            factory.CreateClient();

        var device =
            new ModbusDeviceUpsertRequest(
                "device01",
                "Device",
                false,
                "127.0.0.1",
                502,
                1,
                1000,
                1000);

        var first =
            await client.PostAsJsonAsync(
                "/api/configuration/modbus/devices",
                device);
        var second =
            await client.PostAsJsonAsync(
                "/api/configuration/modbus/devices",
                device);

        Assert.AreEqual(
            HttpStatusCode.Created,
            first.StatusCode);
        Assert.AreEqual(
            HttpStatusCode.Conflict,
            second.StatusCode);

        var tag =
            new ModbusTagUpsertRequest(
                "shared.tag",
                "Tag",
                100,
                false);

        var firstTag =
            await client.PostAsJsonAsync(
                "/api/configuration/modbus/devices/device01/tags",
                tag);
        var secondTag =
            await client.PostAsJsonAsync(
                "/api/configuration/modbus/devices/device01/tags",
                tag);

        Assert.AreEqual(
            HttpStatusCode.Created,
            firstTag.StatusCode);
        Assert.AreEqual(
            HttpStatusCode.Conflict,
            secondTag.StatusCode);
    }

    [TestMethod]
    public async Task ConfigurationCrud_InvalidTag_ReturnsBadRequest()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();
        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);
        using var client =
            factory.CreateClient();

        var device =
            new ModbusDeviceUpsertRequest(
                "device01",
                "Device",
                false,
                "127.0.0.1",
                502,
                1,
                1000,
                1000);

        var deviceResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/modbus/devices",
                device);

        Assert.AreEqual(
            HttpStatusCode.Created,
            deviceResponse.StatusCode);

        var invalidTag =
            new ModbusTagUpsertRequest(
                "device01.invalid",
                "Invalid",
                70000,
                false);

        var response =
            await client.PostAsJsonAsync(
                "/api/configuration/modbus/devices/device01/tags",
                invalidTag);

        Assert.AreEqual(
            HttpStatusCode.BadRequest,
            response.StatusCode);
    }

    [TestMethod]
    public async Task CreateTag_OnRunningServer_AppliesPollingWithoutRestart()
    {
        using var modbusServer =
            new SingleReadModbusTcpServer(
                expectedUnitId: 1,
                expectedAddress: 100,
                registerValue: 2468);

        var serverTask =
            modbusServer.ServeOnceAsync();

        using var database =
            await TestConfigurationDatabase.CreateAsync();
        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);
        using var client =
            factory.CreateClient();

        var device =
            new ModbusDeviceUpsertRequest(
                "device01",
                "Device 01",
                true,
                IPAddress.Loopback.ToString(),
                modbusServer.Port,
                1,
                10000,
                1000);

        var deviceResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/modbus/devices",
                device);

        Assert.AreEqual(
            HttpStatusCode.Created,
            deviceResponse.StatusCode);

        var tag =
            new ModbusTagUpsertRequest(
                "device01.register100",
                "Register 100",
                100,
                false);

        var tagResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/modbus/devices/device01/tags",
                tag);

        Assert.AreEqual(
            HttpStatusCode.Created,
            tagResponse.StatusCode);

        await serverTask.WaitAsync(
            TimeSpan.FromSeconds(2));

        var tagService =
            factory.Services.GetRequiredService<TagService>();

        await WaitUntilAsync(
            () => Equals(
                tagService.Get("device01.register100")?.Value,
                (ushort)2468),
            TimeSpan.FromSeconds(2));
    }

    [TestMethod]
    public async Task ConfigurationMutation_PublishesConfigurationChanged()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();
        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);
        await using var connection =
            CreateConnection(factory);

        var received =
            new TaskCompletionSource(
                TaskCreationOptions.RunContinuationsAsynchronously);

        connection.On(
            RuntimeHubContract.ConfigurationChanged,
            () =>
            {
                received.TrySetResult();
            });

        await connection.StartAsync();

        using var client =
            factory.CreateClient();

        var device =
            new ModbusDeviceUpsertRequest(
                "device01",
                "Device",
                false,
                "127.0.0.1",
                502,
                1,
                1000,
                1000);

        var response =
            await client.PostAsJsonAsync(
                "/api/configuration/modbus/devices",
                device);

        Assert.AreEqual(
            HttpStatusCode.Created,
            response.StatusCode);

        await received.Task.WaitAsync(
            TimeSpan.FromSeconds(2));
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

    private static async Task WaitUntilAsync(
        Func<bool> condition,
        TimeSpan timeout)
    {
        var deadline =
            DateTimeOffset.UtcNow + timeout;

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

            _listener =
                new TcpListener(
                    IPAddress.Loopback,
                    0);
            _listener.Start();

            Port =
                ((IPEndPoint)_listener.LocalEndpoint).Port;
        }

        public int Port { get; }

        public async Task ServeOnceAsync()
        {
            using var client =
                await _listener.AcceptTcpClientAsync();
            using var stream =
                client.GetStream();

            var request = new byte[12];

            await ReadExactlyAsync(
                stream,
                request);

            var unitId = request[6];
            var functionCode = request[7];
            var address =
                (ushort)((request[8] << 8) | request[9]);
            var quantity =
                (ushort)((request[10] << 8) | request[11]);

            Assert.AreEqual(
                _expectedUnitId,
                unitId);
            Assert.AreEqual(
                (byte)3,
                functionCode);
            Assert.AreEqual(
                _expectedAddress,
                address);
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
                (byte)(_registerValue >> 8),
                (byte)_registerValue
            };

            await stream.WriteAsync(
                response);
        }

        private static async Task ReadExactlyAsync(
            NetworkStream stream,
            byte[] buffer)
        {
            var offset = 0;

            while (offset < buffer.Length)
            {
                var read =
                    await stream.ReadAsync(
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
