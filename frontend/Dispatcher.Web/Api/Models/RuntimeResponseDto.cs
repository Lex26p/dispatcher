using System.Text.Json.Serialization;

namespace Dispatcher.Web.Api.Models;

public sealed class RuntimeResponseDto
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

    [JsonPropertyName("service")]
    public RuntimeServiceDto Service { get; set; } = new();

    [JsonPropertyName("runtime")]
    public RuntimeStateDto Runtime { get; set; } = new();

    [JsonPropertyName("telemetry")]
    public RuntimeTelemetryDto Telemetry { get; set; } = new();

    [JsonPropertyName("alarms")]
    public RuntimeAlarmSummaryDto Alarms { get; set; } = new();

    [JsonPropertyName("api")]
    public RuntimeApiDto Api { get; set; } = new();

    public bool IsAvailable =>
        string.Equals(
            Status,
            "available",
            StringComparison.OrdinalIgnoreCase
        );

    public string DisplayRuntimeState =>
        string.IsNullOrWhiteSpace(Runtime.State)
            ? Status
            : Runtime.State;

    public string DisplayServiceName =>
        string.IsNullOrWhiteSpace(Service.Name)
            ? "dispatcher"
            : Service.Name;

    public string DisplayServiceComponent =>
        string.IsNullOrWhiteSpace(Service.Component)
            ? "runtime"
            : Service.Component;
}

public sealed class RuntimeServiceDto
{
    [JsonPropertyName("name")]
    public string Name { get; set; } = string.Empty;

    [JsonPropertyName("component")]
    public string Component { get; set; } = string.Empty;
}

public sealed class RuntimeStateDto
{
    [JsonPropertyName("state")]
    public string State { get; set; } = string.Empty;

    [JsonPropertyName("started")]
    public bool Started { get; set; }

    [JsonPropertyName("configured")]
    public bool Configured { get; set; }

    [JsonPropertyName("accepting_requests")]
    public bool AcceptingRequests { get; set; }
}

public sealed class RuntimeTelemetryDto
{
    [JsonPropertyName("configured")]
    public bool Configured { get; set; }

    [JsonPropertyName("source")]
    public string Source { get; set; } = string.Empty;

    [JsonPropertyName("device_count")]
    public long DeviceCount { get; set; }

    [JsonPropertyName("tag_count")]
    public long TagCount { get; set; }

    [JsonPropertyName("last_batch_sequence")]
    public long? LastBatchSequence { get; set; }

    [JsonPropertyName("last_ingest_timestamp")]
    public string? LastIngestTimestamp { get; set; }
}

public sealed class RuntimeAlarmSummaryDto
{
    [JsonPropertyName("available")]
    public bool Available { get; set; }

    [JsonPropertyName("active_count")]
    public long ActiveCount { get; set; }

    [JsonPropertyName("unacknowledged_count")]
    public long UnacknowledgedCount { get; set; }

    [JsonPropertyName("shelved_count")]
    public long ShelvedCount { get; set; }

    [JsonPropertyName("suppressed_count")]
    public long SuppressedCount { get; set; }
}

public sealed class RuntimeApiDto
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