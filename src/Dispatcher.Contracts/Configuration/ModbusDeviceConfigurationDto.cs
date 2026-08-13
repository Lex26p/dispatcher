namespace Dispatcher.Contracts.Configuration;

public sealed record ModbusDeviceConfigurationDto(
    string DeviceId,
    string Name,
    bool Enabled,
    string Host,
    int Port,
    int UnitId,
    int PollIntervalMilliseconds,
    int RequestTimeoutMilliseconds,
    IReadOnlyList<ModbusTagConfigurationDto> Tags);
