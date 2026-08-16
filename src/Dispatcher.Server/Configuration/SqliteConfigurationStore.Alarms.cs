using System.Globalization;
using Dispatcher.Server.Alarms;

namespace Dispatcher.Server.Configuration;

public sealed partial class SqliteConfigurationStore
{
    public async Task<IReadOnlyList<AlarmDefinitionConfiguration>> LoadAlarmDefinitionsAsync(
        CancellationToken cancellationToken = default)
    {
        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        await using var command =
            connection.CreateCommand();

        command.CommandText =
            """
            SELECT
                alarm_id,
                name,
                enabled,
                tag_id,
                condition,
                threshold_text,
                severity,
                message,
                delay_ms,
                hysteresis_text
            FROM alarm_definitions
            ORDER BY alarm_id;
            """;

        await using var reader =
            await command.ExecuteReaderAsync(
                cancellationToken);

        var definitions =
            new List<AlarmDefinitionConfiguration>();

        while (await reader.ReadAsync(
            cancellationToken))
        {
            var definition =
                new AlarmDefinitionConfiguration(
                    AlarmId:
                        reader.GetString(0),
                    Name:
                        reader.GetString(1),
                    Enabled:
                        reader.GetInt64(2) != 0,
                    TagId:
                        reader.GetString(3),
                    Condition:
                        (AlarmCondition)reader.GetInt32(4),
                    Threshold:
                        ReadNullableDecimal(
                            reader,
                            5,
                            "threshold_text"),
                    Severity:
                        (AlarmSeverity)reader.GetInt32(6),
                    Message:
                        reader.GetString(7),
                    DelayMilliseconds:
                        reader.GetInt32(8),
                    Hysteresis:
                        ReadNullableDecimal(
                            reader,
                            9,
                            "hysteresis_text"));

            AlarmDefinitionValidator.Validate(
                definition);

            definitions.Add(
                definition);
        }

        return definitions;
    }

    public async Task InsertAlarmDefinitionAsync(
        AlarmDefinitionConfiguration definition,
        CancellationToken cancellationToken = default)
    {
        AlarmDefinitionValidator.Validate(
            definition);

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        await using var command =
            connection.CreateCommand();

        command.CommandText =
            """
            INSERT INTO alarm_definitions (
                alarm_id,
                name,
                enabled,
                tag_id,
                condition,
                threshold_text,
                severity,
                message,
                delay_ms,
                hysteresis_text)
            VALUES (
                $alarmId,
                $name,
                $enabled,
                $tagId,
                $condition,
                $thresholdText,
                $severity,
                $message,
                $delayMilliseconds,
                $hysteresisText);
            """;

        AddAlarmDefinitionParameters(
            command,
            definition);

        await command.ExecuteNonQueryAsync(
            cancellationToken);
    }

    public async Task<bool> UpdateAlarmDefinitionAsync(
        AlarmDefinitionConfiguration definition,
        CancellationToken cancellationToken = default)
    {
        AlarmDefinitionValidator.Validate(
            definition);

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        await using var command =
            connection.CreateCommand();

        command.CommandText =
            """
            UPDATE alarm_definitions
            SET
                name = $name,
                enabled = $enabled,
                tag_id = $tagId,
                condition = $condition,
                threshold_text = $thresholdText,
                severity = $severity,
                message = $message,
                delay_ms = $delayMilliseconds,
                hysteresis_text = $hysteresisText
            WHERE alarm_id = $alarmId;
            """;

        AddAlarmDefinitionParameters(
            command,
            definition);

        return await command.ExecuteNonQueryAsync(
            cancellationToken) > 0;
    }

    public async Task<bool> DeleteAlarmDefinitionAsync(
        string alarmId,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            alarmId);

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        await using var command =
            connection.CreateCommand();

        command.CommandText =
            """
            DELETE FROM alarm_definitions
            WHERE alarm_id = $alarmId;
            """;

        command.Parameters.AddWithValue(
            "$alarmId",
            alarmId);

        return await command.ExecuteNonQueryAsync(
            cancellationToken) > 0;
    }

    private static void AddAlarmDefinitionParameters(
        Microsoft.Data.Sqlite.SqliteCommand command,
        AlarmDefinitionConfiguration definition)
    {
        command.Parameters.AddWithValue(
            "$alarmId",
            definition.AlarmId);
        command.Parameters.AddWithValue(
            "$name",
            definition.Name);
        command.Parameters.AddWithValue(
            "$enabled",
            definition.Enabled ? 1 : 0);
        command.Parameters.AddWithValue(
            "$tagId",
            definition.TagId);
        command.Parameters.AddWithValue(
            "$condition",
            (int)definition.Condition);
        command.Parameters.AddWithValue(
            "$thresholdText",
            definition.Threshold is null
                ? DBNull.Value
                : definition.Threshold.Value.ToString(
                    CultureInfo.InvariantCulture));
        command.Parameters.AddWithValue(
            "$severity",
            (int)definition.Severity);
        command.Parameters.AddWithValue(
            "$message",
            definition.Message);
        command.Parameters.AddWithValue(
            "$delayMilliseconds",
            definition.DelayMilliseconds);
        command.Parameters.AddWithValue(
            "$hysteresisText",
            definition.Hysteresis is null
                ? DBNull.Value
                : definition.Hysteresis.Value.ToString(
                    CultureInfo.InvariantCulture));
    }

    private static decimal? ReadNullableDecimal(
        Microsoft.Data.Sqlite.SqliteDataReader reader,
        int ordinal,
        string columnName)
    {
        if (reader.IsDBNull(
                ordinal))
        {
            return null;
        }

        var raw =
            reader.GetString(
                ordinal);

        if (decimal.TryParse(
                raw,
                NumberStyles.Number,
                CultureInfo.InvariantCulture,
                out var value))
        {
            return value;
        }

        throw new InvalidOperationException(
            $"Alarm definition column '{columnName}' contains invalid decimal value '{raw}'.");
    }
}
