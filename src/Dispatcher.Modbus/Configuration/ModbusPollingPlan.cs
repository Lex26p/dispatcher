namespace Dispatcher.Modbus.Configuration;

public sealed record ModbusPollingPlan(
    ModbusTcpDevice Device,
    IReadOnlyList<ModbusHoldingRegisterPoint> Points,
    TimeSpan PollInterval,
    TimeSpan RequestTimeout);
