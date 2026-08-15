using System.Text.Json.Serialization;

namespace Dispatcher.Contracts.Historian;

[JsonConverter(typeof(JsonStringEnumConverter))]
public enum HistoryQueryOrderDto
{
    Ascending,
    Descending
}
