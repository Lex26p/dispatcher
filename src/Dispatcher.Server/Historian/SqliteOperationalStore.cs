using Microsoft.Data.Sqlite;

namespace Dispatcher.Server.Historian;

public sealed class SqliteOperationalStore : IHistorySampleStore
{
    private const int CurrentSchemaVersion = 1;

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
                await CreateSchemaV1Async(
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
            var ticks =
                reader.GetInt64(2);

            samples.Add(
                new HistorySample(
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
                            : reader.GetString(4)));
        }

        return samples;
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

    private static async Task CreateSchemaV1Async(
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

            PRAGMA user_version = 1;
            """;

        await command.ExecuteNonQueryAsync(
            cancellationToken);
    }
}
