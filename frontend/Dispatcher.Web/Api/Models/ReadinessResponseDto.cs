using System.Text.Json.Serialization;

namespace Dispatcher.Web.Api.Models;

public sealed class ReadinessResponseDto
{
    [JsonPropertyName("status")]
    public string Status { get; set; } = string.Empty;

    [JsonPropertyName("ready")]
    public bool Ready { get; set; }

    [JsonPropertyName("readiness_blockers")]
    public bool ReadinessBlockers { get; set; }

    [JsonPropertyName("invalid_checks")]
    public bool InvalidChecks { get; set; }

    [JsonPropertyName("check_count")]
    public int CheckCount { get; set; }

    public bool IsReady =>
        string.Equals(
            Status,
            "ready",
            StringComparison.OrdinalIgnoreCase
        );
}