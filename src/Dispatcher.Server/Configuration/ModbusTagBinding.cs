namespace Dispatcher.Server.Configuration;

public sealed record ModbusTagBinding(
    ModbusDeviceConfiguration Device,
    ModbusTagConfiguration Tag);
