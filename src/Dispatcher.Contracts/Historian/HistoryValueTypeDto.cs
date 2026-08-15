using System.Text.Json.Serialization;

namespace Dispatcher.Contracts.Historian;

[JsonConverter(typeof(JsonStringEnumConverter))]
public enum HistoryValueTypeDto
{
    Null,
    Boolean,
    Int64,
    UInt64,
    Double,
    Decimal,
    String,
    Json
}
