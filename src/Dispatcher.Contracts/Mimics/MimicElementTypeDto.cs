using System.Text.Json.Serialization;

namespace Dispatcher.Contracts.Mimics;

[JsonConverter(typeof(JsonStringEnumConverter))]
public enum MimicElementTypeDto
{
    Text,
    Rectangle,
    Value,
    Indicator,
    Button
}
