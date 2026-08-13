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
        ArgumentNullException.ThrowIfNull(device);
        ArgumentException.ThrowIfNullOrWhiteSpace(device.Host);

        if (device.Port is < 1 or > 65535)
        {
            throw new ArgumentOutOfRangeException(
                nameof(device),
                device.Port,
                "Modbus TCP port must be between 1 and 65535.");
        }

        using var client = new TcpClient();
        client.Connect(device.Host, device.Port);

        var factory = new ModbusFactory();
        var master = factory.CreateMaster(client);

        var registers = master.ReadHoldingRegisters(
            device.UnitId,
            address,
            1);

        return registers[0];
    }
}
