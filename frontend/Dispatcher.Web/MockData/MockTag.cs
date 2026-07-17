namespace Dispatcher.Web.MockData;

public sealed record MockTag(
    string Id,
    string Name,
    string DisplayName,
    string DeviceId,
    string DeviceName,
    string ObjectName,
    string Unit,
    string Value,
    string Quality,
    string SignalType
);