using Dispatcher.Contracts.Historian;

namespace Dispatcher.Server.Historian;

internal static class HistoryContractMapper
{
    public static HistorySampleDto ToDto(
        HistorySample sample)
    {
        ArgumentNullException.ThrowIfNull(
            sample);

        return new HistorySampleDto(
            sample.Timestamp,
            MapValueType(
                sample.ValueType),
            sample.ValueText);
    }

    private static HistoryValueTypeDto MapValueType(
        HistoryValueType valueType)
    {
        return valueType switch
        {
            HistoryValueType.Null =>
                HistoryValueTypeDto.Null,
            HistoryValueType.Boolean =>
                HistoryValueTypeDto.Boolean,
            HistoryValueType.Int64 =>
                HistoryValueTypeDto.Int64,
            HistoryValueType.UInt64 =>
                HistoryValueTypeDto.UInt64,
            HistoryValueType.Double =>
                HistoryValueTypeDto.Double,
            HistoryValueType.Decimal =>
                HistoryValueTypeDto.Decimal,
            HistoryValueType.String =>
                HistoryValueTypeDto.String,
            HistoryValueType.Json =>
                HistoryValueTypeDto.Json,
            _ =>
                throw new ArgumentOutOfRangeException(
                    nameof(valueType),
                    valueType,
                    null)
        };
    }
}
