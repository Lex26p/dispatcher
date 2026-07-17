namespace Dispatcher.Web.MockData;

public sealed record MockAlarm(
    string Id,
    string Time,
    string Level,
    string ObjectId,
    string ObjectName,
    string DeviceName,
    string TagName,
    string Message,
    string State,
    bool Acknowledged,
    bool Shelved,
    bool Suppressed
);