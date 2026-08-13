namespace Dispatcher.Server.Configuration;

public sealed record ModbusTagConfiguration(
    string TagId,
    string Name,
    int Address,
    bool Writable);
