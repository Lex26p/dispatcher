using System.Text.Json.Serialization;

namespace Dispatcher.Contracts.Devices;

[JsonConverter(typeof(JsonStringEnumConverter))]
public enum DeviceConnectionStatusDto
{
    Unknown,
    Online,
    Offline
}
