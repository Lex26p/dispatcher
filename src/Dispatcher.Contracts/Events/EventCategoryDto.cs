using System.Text.Json.Serialization;

namespace Dispatcher.Contracts.Events;

[JsonConverter(typeof(JsonStringEnumConverter))]
public enum EventCategoryDto
{
    System,
    Device,
    Command,
    Configuration
}
