using System.Text.Json.Serialization;

namespace Dispatcher.Contracts.Alarms;

[JsonConverter(typeof(JsonStringEnumConverter))]
public enum AlarmRuntimeStateDto
{
    Normal,
    ActiveUnacknowledged,
    ActiveAcknowledged,
    ReturnedUnacknowledged
}
