using System.Net.Sockets;
using Dispatcher.Modbus.Configuration;
using NModbus;

namespace Dispatcher.Modbus;

public sealed class ModbusTcpRegisterWriter
{
    public async Task WriteHoldingRegisterAsync(
        ModbusTcpDevice device,
        ushort address,
        ushort value,
        TimeSpan requestTimeout,
        CancellationToken cancellationToken = default)
    {
        ValidateDevice(device);

        if (requestTimeout <= TimeSpan.Zero)
        {
            throw new ArgumentOutOfRangeException(
                nameof(requestTimeout),
                requestTimeout,
                "Request timeout must be greater than zero.");
        }

        using var client = new TcpClient();

        using (var connectTimeout = CancellationTokenSource.CreateLinkedTokenSource(
                   cancellationToken))
        {
            connectTimeout.CancelAfter(requestTimeout);

            try
            {
                await client.ConnectAsync(
                    device.Host,
                    device.Port,
                    connectTimeout.Token);
            }
            catch (OperationCanceledException)
                when (!cancellationToken.IsCancellationRequested)
            {
                throw new TimeoutException(
                    $"Timed out connecting to Modbus TCP device '{device.DeviceId}'.");
            }
        }

        var factory = new ModbusFactory();
        using var master = factory.CreateMaster(client);

        var timeoutMilliseconds = ToTimeoutMilliseconds(requestTimeout);
        master.Transport.ReadTimeout = timeoutMilliseconds;
        master.Transport.WriteTimeout = timeoutMilliseconds;
        master.Transport.Retries = 0;

        try
        {
            await master
                .WriteSingleRegisterAsync(
                    device.UnitId,
                    address,
                    value)
                .WaitAsync(
                    requestTimeout,
                    cancellationToken);
        }
        catch (TimeoutException)
        {
            throw new TimeoutException(
                $"Timed out writing holding register {address} " +
                $"to Modbus TCP device '{device.DeviceId}'.");
        }
    }

    private static void ValidateDevice(ModbusTcpDevice device)
    {
        ArgumentNullException.ThrowIfNull(device);
        ArgumentException.ThrowIfNullOrWhiteSpace(device.DeviceId);
        ArgumentException.ThrowIfNullOrWhiteSpace(device.Host);

        if (device.Port is < 1 or > 65535)
        {
            throw new ArgumentOutOfRangeException(
                nameof(device),
                device.Port,
                "Modbus TCP port must be between 1 and 65535.");
        }
    }

    private static int ToTimeoutMilliseconds(TimeSpan timeout)
    {
        return (int)Math.Clamp(
            Math.Ceiling(timeout.TotalMilliseconds),
            1,
            int.MaxValue);
    }
}
