using System.Globalization;
using System.Text.Json;
using Dispatcher.Core.Tags;

namespace Dispatcher.Server.Historian;

internal static class HistorySampleFactory
{
    public static HistorySample Create(
        TagValue tagValue)
    {
        ArgumentNullException.ThrowIfNull(
            tagValue);

        var (valueType, valueText) =
            Encode(
                tagValue.Value);

        return new HistorySample(
            SampleId: 0,
            TagId: tagValue.TagId,
            Timestamp: tagValue.Timestamp.ToUniversalTime(),
            ValueType: valueType,
            ValueText: valueText);
    }

    private static (HistoryValueType Type, string? Text) Encode(
        object? value)
    {
        return value switch
        {
            null =>
                (HistoryValueType.Null, null),

            bool boolean =>
                (
                    HistoryValueType.Boolean,
                    boolean ? "1" : "0"),

            sbyte number =>
                EncodeInt64(number),

            byte number =>
                EncodeInt64(number),

            short number =>
                EncodeInt64(number),

            ushort number =>
                EncodeInt64(number),

            int number =>
                EncodeInt64(number),

            uint number =>
                EncodeInt64(number),

            long number =>
                EncodeInt64(number),

            ulong number =>
                (
                    HistoryValueType.UInt64,
                    number.ToString(
                        CultureInfo.InvariantCulture)),

            float number =>
                (
                    HistoryValueType.Double,
                    number.ToString(
                        "R",
                        CultureInfo.InvariantCulture)),

            double number =>
                (
                    HistoryValueType.Double,
                    number.ToString(
                        "R",
                        CultureInfo.InvariantCulture)),

            decimal number =>
                (
                    HistoryValueType.Decimal,
                    number.ToString(
                        CultureInfo.InvariantCulture)),

            string text =>
                (HistoryValueType.String, text),

            JsonElement json =>
                EncodeJsonElement(
                    json),

            _ =>
                (
                    HistoryValueType.Json,
                    JsonSerializer.Serialize(
                        value,
                        value.GetType()))
        };
    }

    private static (HistoryValueType Type, string Text) EncodeInt64(
        long value)
    {
        return (
            HistoryValueType.Int64,
            value.ToString(
                CultureInfo.InvariantCulture));
    }

    private static (HistoryValueType Type, string? Text) EncodeJsonElement(
        JsonElement value)
    {
        return value.ValueKind switch
        {
            JsonValueKind.Null or
            JsonValueKind.Undefined =>
                (HistoryValueType.Null, null),

            JsonValueKind.True =>
                (HistoryValueType.Boolean, "1"),

            JsonValueKind.False =>
                (HistoryValueType.Boolean, "0"),

            JsonValueKind.String =>
                (
                    HistoryValueType.String,
                    value.GetString()),

            JsonValueKind.Number
                when value.TryGetInt64(
                    out var signed) =>
                EncodeInt64(
                    signed),

            JsonValueKind.Number
                when value.TryGetUInt64(
                    out var unsigned) =>
                (
                    HistoryValueType.UInt64,
                    unsigned.ToString(
                        CultureInfo.InvariantCulture)),

            JsonValueKind.Number
                when value.TryGetDecimal(
                    out var decimalValue) =>
                (
                    HistoryValueType.Decimal,
                    decimalValue.ToString(
                        CultureInfo.InvariantCulture)),

            JsonValueKind.Number =>
                (
                    HistoryValueType.Double,
                    value.GetDouble().ToString(
                        "R",
                        CultureInfo.InvariantCulture)),

            _ =>
                (
                    HistoryValueType.Json,
                    value.GetRawText())
        };
    }
}
