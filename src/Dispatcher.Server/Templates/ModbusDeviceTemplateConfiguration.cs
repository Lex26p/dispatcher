namespace Dispatcher.Server.Templates;

public sealed record ModbusDeviceTemplateConfiguration(
    string TemplateId,
    string Name,
    int Version,
    IReadOnlyList<TemplateParameterConfiguration> Parameters,
    string DeviceName,
    string? DeviceNameParameterId,
    string HostParameterId,
    string TagIdPrefixParameterId,
    bool Enabled,
    int Port,
    int UnitId,
    int PollIntervalMilliseconds,
    int RequestTimeoutMilliseconds,
    IReadOnlyList<ModbusTagTemplateConfiguration> Tags);
