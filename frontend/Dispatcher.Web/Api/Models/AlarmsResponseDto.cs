using System.Text.Json.Serialization;

namespace Dispatcher.Web.Api.Models;

public sealed class AlarmsResponseDto
{
    [JsonPropertyName("schema_version")]
    public long SchemaVersion { get; set; }

    [JsonPropertyName("status")]
    public string Status { get; set; } = string.Empty;

    [JsonPropertyName("endpoint")]
    public string Endpoint { get; set; } = string.Empty;

    [JsonPropertyName("path")]
    public string Path { get; set; } = string.Empty;

    [JsonPropertyName("method")]
    public string Method { get; set; } = string.Empty;

    [JsonPropertyName("source")]
    public string Source { get; set; } = string.Empty;

    [JsonPropertyName("summary")]
    public AlarmSummaryDto Summary { get; set; } = new();

    [JsonPropertyName("severity")]
    public AlarmSeveritySummaryDto Severity { get; set; } = new();

    [JsonPropertyName("states")]
    public AlarmStateSummaryDto States { get; set; } = new();

    [JsonPropertyName("items")]
    public List<AlarmItemDto> Items { get; set; } = [];

    [JsonPropertyName("api")]
    public AlarmApiDto Api { get; set; } = new();

    public long Count =>
        Summary.TotalCount > 0
            ? Summary.TotalCount
            : Items.Count;

    public bool IsAvailable =>
        string.Equals(
            Status,
            "available",
            StringComparison.OrdinalIgnoreCase
        )
        || Summary.Available;

    public bool HasActiveAlarms =>
        Count > 0
        || Summary.ActiveCount > 0
        || States.ActiveCount > 0;
}

public sealed class AlarmSummaryDto
{
    [JsonPropertyName("available")]
    public bool Available { get; set; }

    [JsonPropertyName("total_count")]
    public long TotalCount { get; set; }

    [JsonPropertyName("active_count")]
    public long ActiveCount { get; set; }

    [JsonPropertyName("unacknowledged_count")]
    public long UnacknowledgedCount { get; set; }

    [JsonPropertyName("shelved_count")]
    public long ShelvedCount { get; set; }

    [JsonPropertyName("suppressed_count")]
    public long SuppressedCount { get; set; }

    [JsonPropertyName("inhibited_count")]
    public long InhibitedCount { get; set; }
}

public sealed class AlarmSeveritySummaryDto
{
    [JsonPropertyName("critical_count")]
    public long CriticalCount { get; set; }

    [JsonPropertyName("high_count")]
    public long HighCount { get; set; }

    [JsonPropertyName("medium_count")]
    public long MediumCount { get; set; }

    [JsonPropertyName("low_count")]
    public long LowCount { get; set; }

    [JsonPropertyName("info_count")]
    public long InfoCount { get; set; }
}

public sealed class AlarmStateSummaryDto
{
    [JsonPropertyName("active_count")]
    public long ActiveCount { get; set; }

    [JsonPropertyName("acknowledged_count")]
    public long AcknowledgedCount { get; set; }

    [JsonPropertyName("unacknowledged_count")]
    public long UnacknowledgedCount { get; set; }

    [JsonPropertyName("shelved_count")]
    public long ShelvedCount { get; set; }

    [JsonPropertyName("suppressed_count")]
    public long SuppressedCount { get; set; }

    [JsonPropertyName("inhibited_count")]
    public long InhibitedCount { get; set; }
}

public sealed class AlarmItemDto
{
    [JsonPropertyName("id")]
    public string Id { get; set; } = string.Empty;

    [JsonPropertyName("name")]
    public string Name { get; set; } = string.Empty;

    [JsonPropertyName("tag")]
    public string Tag { get; set; } = string.Empty;

    [JsonPropertyName("severity")]
    public string Severity { get; set; } = string.Empty;

    [JsonPropertyName("state")]
    public string State { get; set; } = string.Empty;

    [JsonPropertyName("message")]
    public string Message { get; set; } = string.Empty;

    [JsonPropertyName("source")]
    public string Source { get; set; } = string.Empty;

    [JsonPropertyName("active")]
    public bool Active { get; set; }

    [JsonPropertyName("acknowledged")]
    public bool Acknowledged { get; set; }

    [JsonPropertyName("shelved")]
    public bool Shelved { get; set; }

    [JsonPropertyName("suppressed")]
    public bool Suppressed { get; set; }

    [JsonPropertyName("inhibited")]
    public bool Inhibited { get; set; }
}

public sealed class AlarmApiDto
{
    [JsonPropertyName("endpoint")]
    public string Endpoint { get; set; } = string.Empty;

    [JsonPropertyName("path")]
    public string Path { get; set; } = string.Empty;

    [JsonPropertyName("method")]
    public string Method { get; set; } = string.Empty;

    [JsonPropertyName("source")]
    public string Source { get; set; } = string.Empty;
}