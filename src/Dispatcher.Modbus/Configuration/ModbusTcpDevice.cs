namespace Dispatcher.Modbus.Configuration;

public sealed record ModbusTcpDevice(
    string Host,
    int Port,
    byte UnitId);
