using System.Text.Json.Serialization;

namespace Dispatcher.Web.Api.Models;

public sealed class HealthResponseDto
{
    [JsonPropertyName("status")]
    public string Status { get; set; } = string.Empty;

    [JsonPropertyName("ready")]
    public bool Ready { get; set; }

    [JsonPropertyName("check_count")]
    public int CheckCount { get; set; }

    [JsonPropertyName("healthy_count")]
    public int HealthyCount { get; set; }

    [JsonPropertyName("degraded_count")]
    public int DegradedCount { get; set; }

    [JsonPropertyName("unhealthy_count")]
    public int UnhealthyCount { get; set; }

    [JsonPropertyName("invalid_count")]
    public int InvalidCount { get; set; }

    public bool IsHealthy =>
        string.Equals(
            Status,
            "healthy",
            StringComparison.OrdinalIgnoreCase
        );

    public bool IsDegraded =>
        string.Equals(
            Status,
            "degraded",
            StringComparison.OrdinalIgnoreCase
        );

    public bool IsUnhealthy =>
        string.Equals(
            Status,
            "unhealthy",
            StringComparison.OrdinalIgnoreCase
        );
}