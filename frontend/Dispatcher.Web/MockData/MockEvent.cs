namespace Dispatcher.Web.MockData;

public sealed record MockEvent(
    string Id,
    string Time,
    string Type,
    string Level,
    string ObjectName,
    string? UserName,
    string Message
);