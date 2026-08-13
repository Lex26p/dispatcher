namespace Dispatcher.Contracts.Configuration;

public sealed record ModbusDeviceUpsertRequest(
    string DeviceId,
    string Name,
    bool Enabled,
    string Host,
    int Port,
    int UnitId,
    int PollIntervalMilliseconds,
    int RequestTimeoutMilliseconds);
