namespace Dispatcher.Contracts.Templates;

public sealed record SnmpDeviceTemplateDto(
    string TemplateId,
    string Name,
    int Version,
    IReadOnlyList<TemplateParameterDto> Parameters,
    string DeviceName,
    string? DeviceNameParameterId,
    string HostParameterId,
    string CommunityParameterId,
    string TagIdPrefixParameterId,
    bool Enabled,
    int Port,
    int PollIntervalMilliseconds,
    int RequestTimeoutMilliseconds,
    IReadOnlyList<SnmpTagTemplateDto> Tags);
