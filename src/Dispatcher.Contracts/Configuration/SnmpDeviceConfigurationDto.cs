namespace Dispatcher.Contracts.Configuration;

public sealed record SnmpDeviceConfigurationDto(
    string DeviceId,
    string Name,
    bool Enabled,
    string Host,
    int Port,
    string Community,
    int PollIntervalMilliseconds,
    int RequestTimeoutMilliseconds,
    IReadOnlyList<SnmpTagConfigurationDto> Tags);
