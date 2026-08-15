using System.Text.Json.Serialization;

namespace Dispatcher.Contracts.Events;

[JsonConverter(typeof(JsonStringEnumConverter))]
public enum EventSeverityDto
{
    Information,
    Warning,
    Error
}
