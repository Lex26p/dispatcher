namespace Dispatcher.Contracts.Templates;

public sealed record ModbusDeviceTemplateDto(
    string TemplateId,
    string Name,
    int Version,
    IReadOnlyList<TemplateParameterDto> Parameters,
    string DeviceName,
    string? DeviceNameParameterId,
    string HostParameterId,
    string TagIdPrefixParameterId,
    bool Enabled,
    int Port,
    int UnitId,
    int PollIntervalMilliseconds,
    int RequestTimeoutMilliseconds,
    IReadOnlyList<ModbusTagTemplateDto> Tags);
