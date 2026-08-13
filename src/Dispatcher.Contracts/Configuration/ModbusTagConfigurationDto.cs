namespace Dispatcher.Contracts.Configuration;

public sealed record ModbusTagConfigurationDto(
    string TagId,
    string Name,
    int Address,
    bool Writable);
