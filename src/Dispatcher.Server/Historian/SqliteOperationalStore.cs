using System.Text;
using Dispatcher.Server.Events;
using Microsoft.Data.Sqlite;

namespace Dispatcher.Server.Historian;

public sealed class SqliteOperationalStore : IHistorySampleStore, IEventJournalStore
{
    private const int CurrentSchemaVersion = 3;

    private readonly string _connectionString;

    public SqliteOperationalStore(
        string databasePath)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            databasePath);

        DatabasePath =
            Path.GetFullPath(
                databasePath);

        _connectionString =
            new SqliteConnectionStringBuilder
            {
                DataSource = DatabasePath,
                Pooling = false
            }
            .ToString();
    }

    public string DatabasePath { get; }

    public async Task InitializeAsync(
        CancellationToken cancellationToken = default)
    {
        var directory =
            Path.GetDirectoryName(
                DatabasePath);

        if (!string.IsNullOrWhiteSpace(
                directory))
        {
            Directory.CreateDirectory(
                directory);
        }

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        var schemaVersion =
            await GetSchemaVersionAsync(
                connection,
                cancellationToken);

        switch (schemaVersion)
        {
            case 0:
                await CreateSchemaV3Async(
                    connection,
                    cancellationToken);
                return;

            case 1:
                await MigrateV1ToV2Async(
                    connection,
                    cancellationToken);
                await MigrateV2ToV3Async(
                    connection,
                    cancellationToken);
                return;

            case 2:
                await MigrateV2ToV3Async(
                    connection,
                    cancellationToken);
                return;

            case CurrentSchemaVersion:
                return;

            default:
                throw new InvalidOperationException(
                    $"Unsupported operational database schema version {schemaVersion}. " +
                    $"Expected {CurrentSchemaVersion}.");
        }
    }

    public async Task AppendAsync(
        IReadOnlyList<HistorySample> samples,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(
            samples);

        if (samples.Count == 0)
        {
            return;
        }

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        using var transaction =
            connection.BeginTransaction();

        await using var command =
            connection.CreateCommand();

        command.Transaction =
            transaction;
        command.CommandText =
            """
            INSERT INTO history_samples (
                tag_id,
                timestamp_utc_ticks,
                value_type,
                value_text)
            VALUES (
                $tagId,
                $timestampUtcTicks,
                $valueType,
                $valueText);
            """;

        var tagIdParameter =
            command.Parameters.Add(
                "$tagId",
                SqliteType.Text);

        var timestampParameter =
            command.Parameters.Add(
                "$timestampUtcTicks",
                SqliteType.Integer);

        var valueTypeParameter =
            command.Parameters.Add(
                "$valueType",
                SqliteType.Integer);

        var valueTextParameter =
            command.Parameters.Add(
                "$valueText",
                SqliteType.Text);

        foreach (var sample in samples)
        {
            ArgumentNullException.ThrowIfNull(
                sample);
            ArgumentException.ThrowIfNullOrWhiteSpace(
                sample.TagId);

            tagIdParameter.Value =
                sample.TagId;

            timestampParameter.Value =
                sample.Timestamp
                    .UtcDateTime
                    .Ticks;

            valueTypeParameter.Value =
                (int)sample.ValueType;

            valueTextParameter.Value =
                sample.ValueText is null
                    ? DBNull.Value
                    : sample.ValueText;

            await command.ExecuteNonQueryAsync(
                cancellationToken);
        }

        transaction.Commit();
    }

    public async Task<IReadOnlyList<EventRecord>> AppendEventsAsync(
        IReadOnlyList<EventRecord> events,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(
            events);

        if (events.Count == 0)
        {
            return Array.Empty<EventRecord>();
        }

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        using var transaction =
            connection.BeginTransaction();

        await using var command =
            connection.CreateCommand();

        command.Transaction =
            transaction;
        command.CommandText =
            """
            INSERT INTO events (
                timestamp_utc_ticks,
                category,
                type,
                severity,
                source,
                message,
                data_json,
                actor_user_id,
                actor_user_name)
            VALUES (
                $timestampUtcTicks,
                $category,
                $type,
                $severity,
                $source,
                $message,
                $dataJson,
                $actorUserId,
                $actorUserName);
            """;

        var timestampParameter =
            command.Parameters.Add(
                "$timestampUtcTicks",
                SqliteType.Integer);

        var categoryParameter =
            command.Parameters.Add(
                "$category",
                SqliteType.Integer);

        var typeParameter =
            command.Parameters.Add(
                "$type",
                SqliteType.Text);

        var severityParameter =
            command.Parameters.Add(
                "$severity",
                SqliteType.Integer);

        var sourceParameter =
            command.Parameters.Add(
                "$source",
                SqliteType.Text);

        var messageParameter =
            command.Parameters.Add(
                "$message",
                SqliteType.Text);

        var dataJsonParameter =
            command.Parameters.Add(
                "$dataJson",
                SqliteType.Text);

        var actorUserIdParameter =
            command.Parameters.Add(
                "$actorUserId",
                SqliteType.Text);

        var actorUserNameParameter =
            command.Parameters.Add(
                "$actorUserName",
                SqliteType.Text);

        await using var idCommand =
            connection.CreateCommand();

        idCommand.Transaction =
            transaction;
        idCommand.CommandText =
            "SELECT last_insert_rowid();";

        var persisted =
            new List<EventRecord>(
                events.Count);

        foreach (var record in events)
        {
            ArgumentNullException.ThrowIfNull(
                record);
            ArgumentException.ThrowIfNullOrWhiteSpace(
                record.Type);
            ArgumentException.ThrowIfNullOrWhiteSpace(
                record.Source);
            ArgumentException.ThrowIfNullOrWhiteSpace(
                record.Message);

            timestampParameter.Value =
                record.Timestamp
                    .UtcDateTime
                    .Ticks;

            categoryParameter.Value =
                (int)record.Category;

            typeParameter.Value =
                record.Type;

            severityParameter.Value =
                (int)record.Severity;

            sourceParameter.Value =
                record.Source;

            messageParameter.Value =
                record.Message;

            dataJsonParameter.Value =
                record.DataJson is null
                    ? DBNull.Value
                    : record.DataJson;

            actorUserIdParameter.Value =
                record.ActorUserId is null
                    ? DBNull.Value
                    : record.ActorUserId;

            actorUserNameParameter.Value =
                record.ActorUserName is null
                    ? DBNull.Value
                    : record.ActorUserName;

            await command.ExecuteNonQueryAsync(
                cancellationToken);

            var eventId =
                Convert.ToInt64(
                    await idCommand.ExecuteScalarAsync(
                        cancellationToken));

            persisted.Add(
                record with
                {
                    EventId =
                        eventId,
                    Timestamp =
                        record.Timestamp.ToUniversalTime()
                });
        }

        transaction.Commit();

        return persisted;
    }

    public async Task<IReadOnlyList<EventRecord>> QueryEventsAsync(
        DateTimeOffset from,
        DateTimeOffset to,
        EventCategory? category,
        EventSeverity? severity,
        string? source,
        string? text,
        int offset,
        int limit,
        CancellationToken cancellationToken = default)
    {
        if (from > to)
        {
            throw new ArgumentException(
                "'from' must be less than or equal to 'to'.");
        }

        if (offset < 0)
        {
            throw new ArgumentOutOfRangeException(
                nameof(offset),
                "Event query offset cannot be negative.");
        }

        if (limit <= 0)
        {
            throw new ArgumentOutOfRangeException(
                nameof(limit),
                "Event query limit must be greater than zero.");
        }

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        await using var command =
            connection.CreateCommand();

        var sql =
            new StringBuilder(
                """
                SELECT
                    event_id,
                    timestamp_utc_ticks,
                    category,
                    type,
                    severity,
                    source,
                    message,
                    data_json,
                    actor_user_id,
                    actor_user_name
                FROM events
                WHERE timestamp_utc_ticks >= $fromUtcTicks
                  AND timestamp_utc_ticks <= $toUtcTicks
                """);

        sql.AppendLine();

        command.Parameters.AddWithValue(
            "$fromUtcTicks",
            from
                .UtcDateTime
                .Ticks);

        command.Parameters.AddWithValue(
            "$toUtcTicks",
            to
                .UtcDateTime
                .Ticks);

        if (category is not null)
        {
            sql.AppendLine(
                "  AND category = $category");

            command.Parameters.AddWithValue(
                "$category",
                (int)category.Value);
        }

        if (severity is not null)
        {
            sql.AppendLine(
                "  AND severity = $severity");

            command.Parameters.AddWithValue(
                "$severity",
                (int)severity.Value);
        }

        if (!string.IsNullOrWhiteSpace(
                source))
        {
            sql.AppendLine(
                "  AND source = $source");

            command.Parameters.AddWithValue(
                "$source",
                source);
        }

        if (!string.IsNullOrWhiteSpace(
                text))
        {
            sql.AppendLine(
                """
                  AND (
                      instr(type, $text) > 0
                      OR instr(source, $text) > 0
                      OR instr(message, $text) > 0
                      OR instr(COALESCE(data_json, ''), $text) > 0
                  )
                """);

            command.Parameters.AddWithValue(
                "$text",
                text);
        }

        sql.Append(
            """
            ORDER BY
                timestamp_utc_ticks DESC,
                event_id DESC
            LIMIT $limit
            OFFSET $offset;
            """);

        command.Parameters.AddWithValue(
            "$limit",
            limit);

        command.Parameters.AddWithValue(
            "$offset",
            offset);

        command.CommandText =
            sql.ToString();

        await using var reader =
            await command.ExecuteReaderAsync(
                cancellationToken);

        var events =
            new List<EventRecord>();

        while (await reader.ReadAsync(
            cancellationToken))
        {
            events.Add(
                ReadEvent(
                    reader));
        }

        return events;
    }

    public async Task<IReadOnlyList<EventRecord>> QueryAlarmEventsAsync(
        DateTimeOffset from,
        DateTimeOffset to,
        int offset,
        int limit,
        CancellationToken cancellationToken = default)
    {
        if (from > to)
        {
            throw new ArgumentException(
                "'from' must be less than or equal to 'to'.");
        }

        if (offset < 0)
        {
            throw new ArgumentOutOfRangeException(
                nameof(offset),
                "Alarm history query offset cannot be negative.");
        }

        if (limit <= 0)
        {
            throw new ArgumentOutOfRangeException(
                nameof(limit),
                "Alarm history query limit must be greater than zero.");
        }

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        await using var command =
            connection.CreateCommand();

        command.CommandText =
            """
            SELECT
                event_id,
                timestamp_utc_ticks,
                category,
                type,
                severity,
                source,
                message,
                data_json,
                actor_user_id,
                actor_user_name
            FROM events
            WHERE timestamp_utc_ticks >= $fromUtcTicks
              AND timestamp_utc_ticks <= $toUtcTicks
              AND type IN (
                  $raisedType,
                  $acknowledgedType,
                  $returnedType)
            ORDER BY
                timestamp_utc_ticks DESC,
                event_id DESC
            LIMIT $limit
            OFFSET $offset;
            """;

        command.Parameters.AddWithValue(
            "$fromUtcTicks",
            from.UtcDateTime.Ticks);
        command.Parameters.AddWithValue(
            "$toUtcTicks",
            to.UtcDateTime.Ticks);
        command.Parameters.AddWithValue(
            "$raisedType",
            EventTypes.AlarmRaised);
        command.Parameters.AddWithValue(
            "$acknowledgedType",
            EventTypes.AlarmAcknowledged);
        command.Parameters.AddWithValue(
            "$returnedType",
            EventTypes.AlarmReturned);
        command.Parameters.AddWithValue(
            "$limit",
            limit);
        command.Parameters.AddWithValue(
            "$offset",
            offset);

        await using var reader =
            await command.ExecuteReaderAsync(
                cancellationToken);

        var events =
            new List<EventRecord>();

        while (await reader.ReadAsync(
            cancellationToken))
        {
            events.Add(
                ReadEvent(
                    reader));
        }

        return events;
    }

    public async Task<IReadOnlyList<EventRecord>> LoadAllEventsAsync(
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
                event_id,
                timestamp_utc_ticks,
                category,
                type,
                severity,
                source,
                message,
                data_json,
                actor_user_id,
                actor_user_name
            FROM events
            ORDER BY event_id;
            """;

        await using var reader =
            await command.ExecuteReaderAsync(
                cancellationToken);

        var events =
            new List<EventRecord>();

        while (await reader.ReadAsync(
            cancellationToken))
        {
            events.Add(
                ReadEvent(
                    reader));
        }

        return events;
    }

    public async Task<IReadOnlyList<HistorySample>> QueryAsync(
        string tagId,
        DateTimeOffset from,
        DateTimeOffset to,
        bool ascending,
        int limit,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            tagId);

        if (from > to)
        {
            throw new ArgumentException(
                "'from' must be less than or equal to 'to'.");
        }

        if (limit <= 0)
        {
            throw new ArgumentOutOfRangeException(
                nameof(limit),
                "Query limit must be greater than zero.");
        }

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        await using var command =
            connection.CreateCommand();

        command.CommandText =
            ascending
                ? """
                  SELECT
                      sample_id,
                      tag_id,
                      timestamp_utc_ticks,
                      value_type,
                      value_text
                  FROM history_samples
                  WHERE tag_id = $tagId
                    AND timestamp_utc_ticks >= $fromUtcTicks
                    AND timestamp_utc_ticks <= $toUtcTicks
                  ORDER BY
                      timestamp_utc_ticks ASC,
                      sample_id ASC
                  LIMIT $limit;
                  """
                : """
                  SELECT
                      sample_id,
                      tag_id,
                      timestamp_utc_ticks,
                      value_type,
                      value_text
                  FROM history_samples
                  WHERE tag_id = $tagId
                    AND timestamp_utc_ticks >= $fromUtcTicks
                    AND timestamp_utc_ticks <= $toUtcTicks
                  ORDER BY
                      timestamp_utc_ticks DESC,
                      sample_id DESC
                  LIMIT $limit;
                  """;

        command.Parameters.AddWithValue(
            "$tagId",
            tagId);
        command.Parameters.AddWithValue(
            "$fromUtcTicks",
            from
                .UtcDateTime
                .Ticks);
        command.Parameters.AddWithValue(
            "$toUtcTicks",
            to
                .UtcDateTime
                .Ticks);
        command.Parameters.AddWithValue(
            "$limit",
            limit);

        await using var reader =
            await command.ExecuteReaderAsync(
                cancellationToken);

        var samples =
            new List<HistorySample>();

        while (await reader.ReadAsync(
            cancellationToken))
        {
            samples.Add(
                ReadSample(
                    reader));
        }

        return samples;
    }

    public async Task<int> DeleteBeforeAsync(
        string tagId,
        DateTimeOffset cutoff,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            tagId);

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        await using var command =
            connection.CreateCommand();

        command.CommandText =
            """
            DELETE FROM history_samples
            WHERE tag_id = $tagId
              AND timestamp_utc_ticks < $cutoffUtcTicks;
            """;

        command.Parameters.AddWithValue(
            "$tagId",
            tagId);

        command.Parameters.AddWithValue(
            "$cutoffUtcTicks",
            cutoff
                .UtcDateTime
                .Ticks);

        return await command.ExecuteNonQueryAsync(
            cancellationToken);
    }

    public async Task<IReadOnlyList<HistorySample>> LoadAllAsync(
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
                sample_id,
                tag_id,
                timestamp_utc_ticks,
                value_type,
                value_text
            FROM history_samples
            ORDER BY sample_id;
            """;

        await using var reader =
            await command.ExecuteReaderAsync(
                cancellationToken);

        var samples =
            new List<HistorySample>();

        while (await reader.ReadAsync(
            cancellationToken))
        {
            samples.Add(
                ReadSample(
                    reader));
        }

        return samples;
    }

    private static EventRecord ReadEvent(
        SqliteDataReader reader)
    {
        var ticks =
            reader.GetInt64(1);

        return new EventRecord(
            EventId:
                reader.GetInt64(0),
            Timestamp:
                new DateTimeOffset(
                    new DateTime(
                        ticks,
                        DateTimeKind.Utc)),
            Category:
                (EventCategory)reader.GetInt32(2),
            Type:
                reader.GetString(3),
            Severity:
                (EventSeverity)reader.GetInt32(4),
            Source:
                reader.GetString(5),
            Message:
                reader.GetString(6),
            DataJson:
                reader.IsDBNull(7)
                    ? null
                    : reader.GetString(7),
            ActorUserId:
                reader.IsDBNull(8)
                    ? null
                    : reader.GetString(8),
            ActorUserName:
                reader.IsDBNull(9)
                    ? null
                    : reader.GetString(9));
    }

    private static HistorySample ReadSample(
        SqliteDataReader reader)
    {
        var ticks =
            reader.GetInt64(2);

        return new HistorySample(
            SampleId:
                reader.GetInt64(0),
            TagId:
                reader.GetString(1),
            Timestamp:
                new DateTimeOffset(
                    new DateTime(
                        ticks,
                        DateTimeKind.Utc)),
            ValueType:
                (HistoryValueType)reader.GetInt32(3),
            ValueText:
                reader.IsDBNull(4)
                    ? null
                    : reader.GetString(4));
    }

    private async Task<SqliteConnection> OpenConnectionAsync(
        CancellationToken cancellationToken)
    {
        var connection =
            new SqliteConnection(
                _connectionString);

        await connection.OpenAsync(
            cancellationToken);

        await using var command =
            connection.CreateCommand();

        command.CommandText =
            "PRAGMA busy_timeout = 5000;";

        await command.ExecuteNonQueryAsync(
            cancellationToken);

        return connection;
    }

    private static async Task<int> GetSchemaVersionAsync(
        SqliteConnection connection,
        CancellationToken cancellationToken)
    {
        await using var command =
            connection.CreateCommand();

        command.CommandText =
            "PRAGMA user_version;";

        var result =
            await command.ExecuteScalarAsync(
                cancellationToken);

        return Convert.ToInt32(
            result);
    }

    private static async Task MigrateV1ToV2Async(
        SqliteConnection connection,
        CancellationToken cancellationToken)
    {
        using var transaction =
            connection.BeginTransaction();

        await using var command =
            connection.CreateCommand();

        command.Transaction =
            transaction;
        command.CommandText =
            """
            CREATE TABLE IF NOT EXISTS events (
                event_id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
                timestamp_utc_ticks INTEGER NOT NULL,
                category INTEGER NOT NULL CHECK (category BETWEEN 0 AND 3),
                type TEXT NOT NULL,
                severity INTEGER NOT NULL CHECK (severity BETWEEN 0 AND 2),
                source TEXT NOT NULL,
                message TEXT NOT NULL,
                data_json TEXT NULL
            );

            CREATE INDEX IF NOT EXISTS ix_events_time
                ON events(
                    timestamp_utc_ticks,
                    event_id);

            CREATE INDEX IF NOT EXISTS ix_events_category_time
                ON events(
                    category,
                    timestamp_utc_ticks,
                    event_id);

            CREATE INDEX IF NOT EXISTS ix_events_severity_time
                ON events(
                    severity,
                    timestamp_utc_ticks,
                    event_id);

            CREATE INDEX IF NOT EXISTS ix_events_source_time
                ON events(
                    source,
                    timestamp_utc_ticks,
                    event_id);

            PRAGMA user_version = 2;
            """;

        await command.ExecuteNonQueryAsync(
            cancellationToken);

        transaction.Commit();
    }

    private static async Task MigrateV2ToV3Async(
        SqliteConnection connection,
        CancellationToken cancellationToken)
    {
        using var transaction =
            connection.BeginTransaction();

        await using var command =
            connection.CreateCommand();

        command.Transaction =
            transaction;
        command.CommandText =
            """
            ALTER TABLE events
                ADD COLUMN actor_user_id TEXT NULL;

            ALTER TABLE events
                ADD COLUMN actor_user_name TEXT NULL;

            PRAGMA user_version = 3;
            """;

        await command.ExecuteNonQueryAsync(
            cancellationToken);

        transaction.Commit();
    }

    private static async Task CreateSchemaV3Async(
        SqliteConnection connection,
        CancellationToken cancellationToken)
    {
        await using var command =
            connection.CreateCommand();

        command.CommandText =
            """
            CREATE TABLE IF NOT EXISTS history_samples (
                sample_id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
                tag_id TEXT NOT NULL,
                timestamp_utc_ticks INTEGER NOT NULL,
                value_type INTEGER NOT NULL CHECK (value_type BETWEEN 0 AND 7),
                value_text TEXT NULL
            );

            CREATE INDEX IF NOT EXISTS ix_history_samples_tag_time
                ON history_samples(
                    tag_id,
                    timestamp_utc_ticks,
                    sample_id);

            CREATE TABLE IF NOT EXISTS events (
                event_id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
                timestamp_utc_ticks INTEGER NOT NULL,
                category INTEGER NOT NULL CHECK (category BETWEEN 0 AND 3),
                type TEXT NOT NULL,
                severity INTEGER NOT NULL CHECK (severity BETWEEN 0 AND 2),
                source TEXT NOT NULL,
                message TEXT NOT NULL,
                data_json TEXT NULL,
                actor_user_id TEXT NULL,
                actor_user_name TEXT NULL
            );

            CREATE INDEX IF NOT EXISTS ix_events_time
                ON events(
                    timestamp_utc_ticks,
                    event_id);

            CREATE INDEX IF NOT EXISTS ix_events_category_time
                ON events(
                    category,
                    timestamp_utc_ticks,
                    event_id);

            CREATE INDEX IF NOT EXISTS ix_events_severity_time
                ON events(
                    severity,
                    timestamp_utc_ticks,
                    event_id);

            CREATE INDEX IF NOT EXISTS ix_events_source_time
                ON events(
                    source,
                    timestamp_utc_ticks,
                    event_id);

            PRAGMA user_version = 3;
            """;

        await command.ExecuteNonQueryAsync(
            cancellationToken);
    }
}
