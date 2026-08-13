namespace Dispatcher.Modbus.Configuration;

public sealed record ModbusHoldingRegisterPoint(
    string TagId,
    ushort Address);
