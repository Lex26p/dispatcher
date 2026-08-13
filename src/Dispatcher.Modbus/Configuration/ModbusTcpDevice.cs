namespace Dispatcher.Modbus.Configuration;

public sealed record ModbusTcpDevice(
    string DeviceId,
    string Host,
    int Port,
    byte UnitId);
