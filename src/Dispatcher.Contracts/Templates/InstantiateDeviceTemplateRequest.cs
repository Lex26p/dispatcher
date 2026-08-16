namespace Dispatcher.Contracts.Templates;

public sealed record InstantiateDeviceTemplateRequest(
    string DeviceId,
    Dictionary<string, string> ParameterValues);
