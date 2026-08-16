using System.Text.Json.Serialization;

namespace Dispatcher.Contracts.Templates;

[JsonConverter(typeof(JsonStringEnumConverter))]
public enum TemplateKindDto
{
    Mimic,
    ModbusDevice,
    SnmpDevice
}
