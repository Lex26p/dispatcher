namespace Dispatcher.Web.MockData;

public sealed record MockDevicePollTestResult(
    int TotalRequests,
    int SuccessfulResponses,
    int FailedRequests,
    int AverageResponseTimeMs,
    int MinResponseTimeMs,
    int MaxResponseTimeMs,
    string? LastError,
    IReadOnlyList<MockDevicePollAttempt> Attempts
);