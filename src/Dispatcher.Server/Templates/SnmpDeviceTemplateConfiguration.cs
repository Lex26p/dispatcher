namespace Dispatcher.Server.Templates;

public sealed record SnmpDeviceTemplateConfiguration(
    string TemplateId,
    string Name,
    int Version,
    IReadOnlyList<TemplateParameterConfiguration> Parameters,
    string DeviceName,
    string? DeviceNameParameterId,
    string HostParameterId,
    string CommunityParameterId,
    string TagIdPrefixParameterId,
    bool Enabled,
    int Port,
    int PollIntervalMilliseconds,
    int RequestTimeoutMilliseconds,
    IReadOnlyList<SnmpTagTemplateConfiguration> Tags);
