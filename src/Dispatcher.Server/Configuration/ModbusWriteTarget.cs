using Dispatcher.Modbus.Configuration;

namespace Dispatcher.Server.Configuration;

public sealed record ModbusWriteTarget(
    ModbusTcpDevice Device,
    ModbusHoldingRegisterPoint Point,
    TimeSpan RequestTimeout);
