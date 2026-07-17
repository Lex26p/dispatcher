namespace Dispatcher.Web.MockData;

public sealed class MockDeviceDiagnosticsService
{
    public MockDevicePingResult CheckConnection(MockDevice device)
    {
        var success = device.Status is not "Нет связи";
        var responseTimeMs = success ? GetBaseResponseTime(device) : 0;

        return new MockDevicePingResult(
            Success: success,
            Endpoint: device.Endpoint,
            ResponseTimeMs: responseTimeMs,
            Error: success ? null : "Устройство недоступно",
            CheckedAt: DateTime.Now
        );
    }

    public MockDevicePollTestResult RunPollTest(MockDevice device, int requestCount)
    {
        var normalizedRequestCount = Math.Clamp(requestCount, 1, 100);
        var attempts = new List<MockDevicePollAttempt>();

        for (var index = 1; index <= normalizedRequestCount; index++)
        {
            var success = IsAttemptSuccessful(device, index);
            var responseTimeMs = success ? GetAttemptResponseTime(device, index) : 0;
            var error = success ? null : GetAttemptError(device, index);

            attempts.Add(
                new MockDevicePollAttempt(
                    Number: index,
                    Success: success,
                    ResponseTimeMs: responseTimeMs,
                    Error: error
                )
            );
        }

        var successfulAttempts =
            attempts
                .Where(item => item.Success)
                .ToList();

        var failedAttempts =
            attempts
                .Where(item => !item.Success)
                .ToList();

        var minResponseTime =
            successfulAttempts.Count > 0
                ? successfulAttempts.Min(item => item.ResponseTimeMs)
                : 0;

        var maxResponseTime =
            successfulAttempts.Count > 0
                ? successfulAttempts.Max(item => item.ResponseTimeMs)
                : 0;

        var averageResponseTime =
            successfulAttempts.Count > 0
                ? (int)Math.Round(successfulAttempts.Average(item => item.ResponseTimeMs))
                : 0;

        return new MockDevicePollTestResult(
            TotalRequests: normalizedRequestCount,
            SuccessfulResponses: successfulAttempts.Count,
            FailedRequests: failedAttempts.Count,
            AverageResponseTimeMs: averageResponseTime,
            MinResponseTimeMs: minResponseTime,
            MaxResponseTimeMs: maxResponseTime,
            LastError: failedAttempts.LastOrDefault()?.Error,
            Attempts: attempts
        );
    }

    private static bool IsAttemptSuccessful(MockDevice device, int attemptNumber)
    {
        if (device.Status is "Нет связи")
        {
            return false;
        }

        if (device.Status is "Авария")
        {
            return attemptNumber % 5 != 0;
        }

        return attemptNumber % 11 != 0;
    }

    private static int GetBaseResponseTime(MockDevice device)
    {
        return device.Protocol switch
        {
            "Modbus TCP" => 32,
            "Modbus RTU" => 86,
            _ => 50
        };
    }

    private static int GetAttemptResponseTime(MockDevice device, int attemptNumber)
    {
        var baseTime = GetBaseResponseTime(device);
        var jitter = (attemptNumber * 7 + device.Id.Length * 3) % 28;

        return baseTime + jitter;
    }

    private static string GetAttemptError(MockDevice device, int attemptNumber)
    {
        if (device.Status is "Нет связи")
        {
            return "connection failed";
        }

        return attemptNumber % 2 == 0
            ? "timeout"
            : "bad response";
    }
}