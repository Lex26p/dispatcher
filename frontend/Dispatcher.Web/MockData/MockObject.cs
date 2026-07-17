namespace Dispatcher.Web.MockData;

public sealed record MockObject(
    string Id,
    string Name,
    string Group,
    string Type,
    string Status,
    int ActiveAlarmCount,
    int WidgetCount,
    string LastEvent,
    bool IsSystemObject
);