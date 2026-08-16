namespace Dispatcher.Contracts.Templates;

public sealed record SnmpDeviceTemplateUpsertRequest(
    string TemplateId,
    string Name,
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
