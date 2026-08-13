using System.Net;
using System.Net.Sockets;

namespace Dispatcher.Modbus.Tests;

internal sealed class TestModbusTcpServer : IDisposable
{
    private readonly TcpListener _listener;
    private readonly byte _expectedUnitId;
    private readonly Func<int, int, ushort, ushort> _valueFactory;

    public TestModbusTcpServer(
        byte expectedUnitId,
        Func<int, int, ushort, ushort> valueFactory)
    {
        _expectedUnitId = expectedUnitId;
        _valueFactory = valueFactory;

        _listener = new TcpListener(IPAddress.Loopback, 0);
        _listener.Start();

        Port = ((IPEndPoint)_listener.LocalEndpoint).Port;
    }

    public int Port { get; }

    public async Task ServeAsync(params int[] requestsPerConnection)
    {
        for (var connectionIndex = 0;
             connectionIndex < requestsPerConnection.Length;
             connectionIndex++)
        {
            using var client = await _listener.AcceptTcpClientAsync();
            using var stream = client.GetStream();

            for (var requestIndex = 0;
                 requestIndex < requestsPerConnection[connectionIndex];
                 requestIndex++)
            {
                var request = new byte[12];
                await ReadExactlyAsync(stream, request);

                var address = ValidateRequest(request);
                var value = _valueFactory(
                    connectionIndex,
                    requestIndex,
                    address);

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
                    (byte)(value >> 8),
                    (byte)value
                };

                await stream.WriteAsync(response);
            }
        }
    }


    public async Task AcceptAndIgnoreOneRequestAsync(TimeSpan holdTime)
    {
        using var client = await _listener.AcceptTcpClientAsync();
        using var stream = client.GetStream();

        var request = new byte[12];
        await ReadExactlyAsync(stream, request);
        ValidateRequest(request);

        await Task.Delay(holdTime);
    }

    private ushort ValidateRequest(byte[] request)
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

        if (quantity != 1)
        {
            throw new InvalidDataException(
                $"Expected one register, received {quantity}.");
        }

        return address;
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
