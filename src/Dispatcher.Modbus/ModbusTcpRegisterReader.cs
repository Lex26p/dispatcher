using System.Net.Sockets;
using Dispatcher.Modbus.Configuration;
using NModbus;

namespace Dispatcher.Modbus;

public sealed class ModbusTcpRegisterReader
{
    public ushort ReadHoldingRegister(
        ModbusTcpDevice device,
        ushort address)
    {
        ValidateDevice(device);

        using var client = new TcpClient();
        client.Connect(device.Host, device.Port);

        var factory = new ModbusFactory();
        using var master = factory.CreateMaster(client);

        var registers = master.ReadHoldingRegisters(
            device.UnitId,
            address,
            1);

        return registers[0];
    }

    public async Task<IReadOnlyDictionary<ushort, ushort>> ReadHoldingRegistersAsync(
        ModbusTcpDevice device,
        IReadOnlyCollection<ushort> addresses,
        TimeSpan requestTimeout,
        CancellationToken cancellationToken = default)
    {
        ValidateDevice(device);
        ArgumentNullException.ThrowIfNull(addresses);

        if (addresses.Count == 0)
        {
            throw new ArgumentException(
                "At least one Modbus address is required.",
                nameof(addresses));
        }

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

        var values = new Dictionary<ushort, ushort>();

        foreach (var address in addresses.Distinct())
        {
            cancellationToken.ThrowIfCancellationRequested();

            ushort[] registers;

            try
            {
                registers = await master
                    .ReadHoldingRegistersAsync(
                        device.UnitId,
                        address,
                        1)
                    .WaitAsync(requestTimeout, cancellationToken);
            }
            catch (TimeoutException)
            {
                throw new TimeoutException(
                    $"Timed out reading holding register {address} " +
                    $"from Modbus TCP device '{device.DeviceId}'.");
            }

            values[address] = registers[0];
        }

        return values;
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
