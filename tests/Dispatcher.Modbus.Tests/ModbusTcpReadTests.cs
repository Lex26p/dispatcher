using System.Net;
using System.Net.Sockets;
using Dispatcher.Core.Tags;
using Dispatcher.Modbus.Configuration;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Modbus.Tests;

[TestClass]
public sealed class ModbusTcpReadTests
{
    [TestMethod]
    public async Task ReadHoldingRegister_FromTcpServer_StoresTagValue()
    {
        using var server = new SingleReadModbusTcpServer(
            expectedUnitId: 1,
            expectedAddress: 100,
            registerValue: 1234);

        var serverTask = server.ServeOnceAsync();

        var tagService = new TagService();
        var reader = new ModbusTcpRegisterReader();
        var readService = new ModbusReadService(tagService, reader);

        var device = new ModbusTcpDevice(
            Host: IPAddress.Loopback.ToString(),
            Port: server.Port,
            UnitId: 1);

        var point = new ModbusHoldingRegisterPoint(
            TagId: "device01.register100",
            Address: 100);

        var result = readService.ReadHoldingRegister(device, point);

        await serverTask;

        Assert.AreEqual("device01.register100", result.TagId);
        Assert.AreEqual((ushort)1234, result.Value);
        Assert.AreEqual(result, tagService.Get("device01.register100"));
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

            _listener = new TcpListener(IPAddress.Loopback, 0);
            _listener.Start();

            Port = ((IPEndPoint)_listener.LocalEndpoint).Port;
        }

        public int Port { get; }

        public async Task ServeOnceAsync()
        {
            using var client = await _listener.AcceptTcpClientAsync();
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
            var address = (ushort)((request[8] << 8) | request[9]);
            var quantity = (ushort)((request[10] << 8) | request[11]);

            if (unitId != _expectedUnitId)
            {
                throw new InvalidDataException(
                    $"Expected unit id {_expectedUnitId}, received {unitId}.");
            }

            if (functionCode != 3)
            {
                throw new InvalidDataException(
                    $"Expected function code 3, received {functionCode}.");
            }

            if (address != _expectedAddress)
            {
                throw new InvalidDataException(
                    $"Expected address {_expectedAddress}, received {address}.");
            }

            if (quantity != 1)
            {
                throw new InvalidDataException(
                    $"Expected one register, received {quantity}.");
            }
        }

        private static async Task ReadExactlyAsync(
            NetworkStream stream,
            byte[] buffer)
        {
            var offset = 0;

            while (offset < buffer.Length)
            {
                var read = await stream.ReadAsync(
                    buffer.AsMemory(offset, buffer.Length - offset));

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
