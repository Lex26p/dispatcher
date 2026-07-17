namespace Dispatcher.Web.MockData;

public sealed record MockDevice(
    string Id,
    string Name,
    string Type,
    string ObjectId,
    string ObjectName,
    string Location,
    string Protocol,
    string Endpoint,
    string Status,
    int TagCount,
    string LastPoll
);