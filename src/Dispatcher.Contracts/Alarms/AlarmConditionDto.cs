using System.Text.Json.Serialization;

namespace Dispatcher.Contracts.Alarms;

[JsonConverter(typeof(JsonStringEnumConverter))]
public enum AlarmConditionDto
{
    DigitalTrue,
    DigitalFalse,
    High,
    Low
}
