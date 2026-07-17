namespace Dispatcher.Web.MockData;

public sealed record MockDevicePollAttempt(
    int Number,
    bool Success,
    int ResponseTimeMs,
    string? Error
);