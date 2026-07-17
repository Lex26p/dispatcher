namespace Dispatcher.Web.MockData;

public sealed record MockDevicePingResult(
    bool Success,
    string Endpoint,
    int ResponseTimeMs,
    string? Error,
    DateTime CheckedAt
);