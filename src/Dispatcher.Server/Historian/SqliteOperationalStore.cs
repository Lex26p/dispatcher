using Dispatcher.Server.Events;
using Microsoft.Data.Sqlite;

namespace Dispatcher.Server.Historian;

public sealed class SqliteOperationalStore : IHistorySampleStore, IEventJournalStore
{
    private const int CurrentSchemaVersion = 2;

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
                await CreateSchemaV2Async(
                    connection,
                    cancellationToken);
                return;

            case 1:
                await MigrateV1ToV2Async(
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

    public async Task AppendEventsAsync(
        IReadOnlyList<EventRecord> events,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(
            events);

        if (events.Count == 0)
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
            INSERT INTO events (
                timestamp_utc_ticks,
                category,
                type,
                severity,
                source,
                message,
                data_json)
            VALUES (
                $timestampUtcTicks,
                $category,
                $type,
                $severity,
                $source,
                $message,
                $dataJson);
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

            await command.ExecuteNonQueryAsync(
                cancellationToken);
        }

        transaction.Commit();
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
                data_json
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
                    : reader.GetString(7));
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

    private static async Task CreateSchemaV2Async(
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
    }
}
