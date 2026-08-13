namespace Dispatcher.Server.Configuration;

public sealed record ModbusDeviceConfiguration(
    string DeviceId,
    string Name,
    bool Enabled,
    string Host,
    int Port,
    int UnitId,
    int PollIntervalMilliseconds,
    int RequestTimeoutMilliseconds,
    IReadOnlyList<ModbusTagConfiguration> Tags);
