namespace Dispatcher.Contracts.Configuration;

public sealed record SnmpDeviceUpsertRequest(
    string DeviceId,
    string Name,
    bool Enabled,
    string Host,
    int Port,
    string Community,
    int PollIntervalMilliseconds,
    int RequestTimeoutMilliseconds);
