namespace Dispatcher.Web.Configuration;

public sealed class DispatcherApiOptions
{
    public const string SectionName = "DispatcherApi";

    public DispatcherApiOptions(
        string baseUrl,
        string healthEndpoint,
        string readinessEndpoint,
        string runtimeEndpoint,
        string alarmsEndpoint,
        int timeoutSeconds
    )
    {
        BaseUrl = NormalizeBaseUrl(baseUrl);
        HealthEndpoint = NormalizeEndpoint(healthEndpoint);
        ReadinessEndpoint = NormalizeEndpoint(readinessEndpoint);
        RuntimeEndpoint = NormalizeEndpoint(runtimeEndpoint);
        AlarmsEndpoint = NormalizeEndpoint(alarmsEndpoint);
        TimeoutSeconds = timeoutSeconds > 0 ? timeoutSeconds : 5;
    }

    public string BaseUrl { get; }

    public string HealthEndpoint { get; }

    public string ReadinessEndpoint { get; }

    public string RuntimeEndpoint { get; }

    public string AlarmsEndpoint { get; }

    public int TimeoutSeconds { get; }

    public Uri BaseUri => new(BaseUrl, UriKind.Absolute);

    public TimeSpan Timeout => TimeSpan.FromSeconds(TimeoutSeconds);

    public string HealthUrl => Combine(BaseUrl, HealthEndpoint);

    public string ReadinessUrl => Combine(BaseUrl, ReadinessEndpoint);

    public string RuntimeUrl => Combine(BaseUrl, RuntimeEndpoint);

    public string AlarmsUrl => Combine(BaseUrl, AlarmsEndpoint);

    public static DispatcherApiOptions Default()
    {
        return new DispatcherApiOptions(
            "http://127.0.0.1:18080",
            "/health",
            "/ready",
            "/api/v1/runtime",
            "/api/v1/alarms",
            5
        );
    }

    public static DispatcherApiOptions FromConfiguration(
        IConfiguration configuration
    )
    {
        var sectionName = SectionName + ":";

        return new DispatcherApiOptions(
            configuration[sectionName + "BaseUrl"] ?? "http://127.0.0.1:18080",
            configuration[sectionName + "HealthEndpoint"] ?? "/health",
            configuration[sectionName + "ReadinessEndpoint"] ?? "/ready",
            configuration[sectionName + "RuntimeEndpoint"] ?? "/api/v1/runtime",
            configuration[sectionName + "AlarmsEndpoint"] ?? "/api/v1/alarms",
            ParseTimeoutSeconds(
                configuration[sectionName + "TimeoutSeconds"]
            )
        );
    }

    private static int ParseTimeoutSeconds(
        string? value
    )
    {
        if (int.TryParse(value, out var parsed)
            && parsed > 0)
        {
            return parsed;
        }

        return 5;
    }

    private static string NormalizeBaseUrl(
        string value
    )
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            return "http://127.0.0.1:18080";
        }

        return value.Trim().TrimEnd('/');
    }

    private static string NormalizeEndpoint(
        string value
    )
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            return "/";
        }

        var trimmed = value.Trim();

        return trimmed.StartsWith(
            "/",
            StringComparison.Ordinal
        )
            ? trimmed
            : "/" + trimmed;
    }

    private static string Combine(
        string baseUrl,
        string endpoint
    )
    {
        return NormalizeBaseUrl(baseUrl) + NormalizeEndpoint(endpoint);
    }
}