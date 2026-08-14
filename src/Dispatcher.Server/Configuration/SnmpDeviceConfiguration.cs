namespace Dispatcher.Server.Configuration;

public sealed record SnmpDeviceConfiguration(
    string DeviceId,
    string Name,
    bool Enabled,
    string Host,
    int Port,
    string Community,
    int PollIntervalMilliseconds,
    int RequestTimeoutMilliseconds,
    IReadOnlyList<SnmpTagConfiguration> Tags);
