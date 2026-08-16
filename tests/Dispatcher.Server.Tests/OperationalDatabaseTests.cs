using Dispatcher.Server.Events;
using Dispatcher.Server.Historian;
using Microsoft.Data.Sqlite;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class OperationalDatabaseTests
{
    [TestMethod]
    public async Task InitializeAsync_CreatesIndependentSchemaVersion3()
    {
        using var database =
            TemporaryOperationalDatabase.Create();

        var store =
            new SqliteOperationalStore(
                database.DatabasePath);

        await store.InitializeAsync();
        await store.InitializeAsync();

        var version =
            await ReadSchemaVersionAsync(
                database.DatabasePath);

        Assert.AreEqual(
            3,
            version);

        var samples =
            await store.LoadAllAsync();
        var events =
            await store.LoadAllEventsAsync();

        Assert.AreEqual(
            0,
            samples.Count);
        Assert.AreEqual(
            0,
            events.Count);
    }

    [TestMethod]
    public async Task InitializeAsync_UnsupportedSchemaVersion_Throws()
    {
        using var database =
            TemporaryOperationalDatabase.Create();

        await using (var connection =
            new SqliteConnection(
                CreateConnectionString(
                    database.DatabasePath)))
        {
            await connection.OpenAsync();

            await using var command =
                connection.CreateCommand();

            command.CommandText =
                "PRAGMA user_version = 99;";

            await command.ExecuteNonQueryAsync();
        }

        var store =
            new SqliteOperationalStore(
                database.DatabasePath);

        try
        {
            await store.InitializeAsync();

            Assert.Fail(
                "Unsupported operational schema version must fail startup.");
        }
        catch (InvalidOperationException)
        {
        }
    }

    [TestMethod]
    public async Task AppendAsync_PreservesTypedHistoryRecords()
    {
        using var database =
            TemporaryOperationalDatabase.Create();

        var store =
            new SqliteOperationalStore(
                database.DatabasePath);

        await store.InitializeAsync();

        var timestamp =
            new DateTimeOffset(
                2026,
                8,
                15,
                12,
                34,
                56,
                TimeSpan.FromHours(3));

        await store.AppendAsync(
            [
                new HistorySample(
                    0,
                    "tag.null",
                    timestamp,
                    HistoryValueType.Null,
                    null),
                new HistorySample(
                    0,
                    "tag.bool",
                    timestamp,
                    HistoryValueType.Boolean,
                    "1"),
                new HistorySample(
                    0,
                    "tag.int",
                    timestamp,
                    HistoryValueType.Int64,
                    "-123"),
                new HistorySample(
                    0,
                    "tag.uint64",
                    timestamp,
                    HistoryValueType.UInt64,
                    ulong.MaxValue.ToString()),
                new HistorySample(
                    0,
                    "tag.double",
                    timestamp,
                    HistoryValueType.Double,
                    "12.5"),
                new HistorySample(
                    0,
                    "tag.decimal",
                    timestamp,
                    HistoryValueType.Decimal,
                    "123.456"),
                new HistorySample(
                    0,
                    "tag.string",
                    timestamp,
                    HistoryValueType.String,
                    "dispatcher")
            ]);

        var loaded =
            await store.LoadAllAsync();

        Assert.AreEqual(
            7,
            loaded.Count);

        Assert.AreEqual(
            HistoryValueType.Null,
            loaded[0].ValueType);
        Assert.IsNull(
            loaded[0].ValueText);

        Assert.AreEqual(
            "1",
            loaded[1].ValueText);
        Assert.AreEqual(
            "-123",
            loaded[2].ValueText);
        Assert.AreEqual(
            ulong.MaxValue.ToString(),
            loaded[3].ValueText);
        Assert.AreEqual(
            "12.5",
            loaded[4].ValueText);
        Assert.AreEqual(
            "123.456",
            loaded[5].ValueText);
        Assert.AreEqual(
            "dispatcher",
            loaded[6].ValueText);

        Assert.IsTrue(
            loaded.All(sample =>
                sample.Timestamp.Offset == TimeSpan.Zero));

        Assert.AreEqual(
            timestamp.ToUniversalTime(),
            loaded[0].Timestamp);
    }

    [TestMethod]
    public async Task InitializeAsync_MigratesVersion1ToVersion3_WithoutLosingHistory()
    {
        using var database =
            TemporaryOperationalDatabase.Create();

        await using (var connection =
            new SqliteConnection(
                CreateConnectionString(
                    database.DatabasePath)))
        {
            await connection.OpenAsync();

            await using var command =
                connection.CreateCommand();

            command.CommandText =
                """
                CREATE TABLE history_samples (
                    sample_id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
                    tag_id TEXT NOT NULL,
                    timestamp_utc_ticks INTEGER NOT NULL,
                    value_type INTEGER NOT NULL CHECK (value_type BETWEEN 0 AND 7),
                    value_text TEXT NULL
                );

                CREATE INDEX ix_history_samples_tag_time
                    ON history_samples(
                        tag_id,
                        timestamp_utc_ticks,
                        sample_id);

                INSERT INTO history_samples (
                    tag_id,
                    timestamp_utc_ticks,
                    value_type,
                    value_text)
                VALUES (
                    'legacy.tag',
                    638908128000000000,
                    6,
                    'legacy');

                PRAGMA user_version = 1;
                """;

            await command.ExecuteNonQueryAsync();
        }

        var store =
            new SqliteOperationalStore(
                database.DatabasePath);

        await store.InitializeAsync();

        Assert.AreEqual(
            3,
            await ReadSchemaVersionAsync(
                database.DatabasePath));

        var samples =
            await store.LoadAllAsync();
        var events =
            await store.LoadAllEventsAsync();

        Assert.AreEqual(
            1,
            samples.Count);
        Assert.AreEqual(
            "legacy.tag",
            samples[0].TagId);
        Assert.AreEqual(
            "legacy",
            samples[0].ValueText);
        Assert.AreEqual(
            0,
            events.Count);
    }

    [TestMethod]
    public async Task InitializeAsync_MigratesVersion2ToVersion3_WithoutLosingEvents()
    {
        using var database =
            TemporaryOperationalDatabase.Create();

        await using (var connection =
            new SqliteConnection(
                CreateConnectionString(
                    database.DatabasePath)))
        {
            await connection.OpenAsync();

            await using var command =
                connection.CreateCommand();

            command.CommandText =
                """
                CREATE TABLE history_samples (
                    sample_id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
                    tag_id TEXT NOT NULL,
                    timestamp_utc_ticks INTEGER NOT NULL,
                    value_type INTEGER NOT NULL CHECK (value_type BETWEEN 0 AND 7),
                    value_text TEXT NULL
                );

                CREATE INDEX ix_history_samples_tag_time
                    ON history_samples(
                        tag_id,
                        timestamp_utc_ticks,
                        sample_id);

                CREATE TABLE events (
                    event_id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
                    timestamp_utc_ticks INTEGER NOT NULL,
                    category INTEGER NOT NULL CHECK (category BETWEEN 0 AND 3),
                    type TEXT NOT NULL,
                    severity INTEGER NOT NULL CHECK (severity BETWEEN 0 AND 2),
                    source TEXT NOT NULL,
                    message TEXT NOT NULL,
                    data_json TEXT NULL
                );

                CREATE INDEX ix_events_time
                    ON events(timestamp_utc_ticks, event_id);
                CREATE INDEX ix_events_category_time
                    ON events(category, timestamp_utc_ticks, event_id);
                CREATE INDEX ix_events_severity_time
                    ON events(severity, timestamp_utc_ticks, event_id);
                CREATE INDEX ix_events_source_time
                    ON events(source, timestamp_utc_ticks, event_id);

                INSERT INTO events (
                    timestamp_utc_ticks,
                    category,
                    type,
                    severity,
                    source,
                    message,
                    data_json)
                VALUES (
                    638908128000000000,
                    2,
                    'LegacyEvent',
                    0,
                    'legacy',
                    'Legacy event.',
                    '{"legacy":true}');

                PRAGMA user_version = 2;
                """;

            await command.ExecuteNonQueryAsync();
        }

        var store =
            new SqliteOperationalStore(
                database.DatabasePath);

        await store.InitializeAsync();

        Assert.AreEqual(
            3,
            await ReadSchemaVersionAsync(
                database.DatabasePath));

        var events =
            await store.LoadAllEventsAsync();

        Assert.AreEqual(
            1,
            events.Count);
        Assert.AreEqual(
            "LegacyEvent",
            events[0].Type);
        Assert.AreEqual(
            """{"legacy":true}""",
            events[0].DataJson);
        Assert.IsNull(
            events[0].ActorUserId);
        Assert.IsNull(
            events[0].ActorUserName);
    }

    [TestMethod]
    public async Task AppendEventsAsync_PreservesImmutableEventRecord()
    {
        using var database =
            TemporaryOperationalDatabase.Create();

        var store =
            new SqliteOperationalStore(
                database.DatabasePath);

        await store.InitializeAsync();

        var timestamp =
            new DateTimeOffset(
                2026,
                8,
                15,
                19,
                0,
                0,
                TimeSpan.FromHours(3));

        await store.AppendEventsAsync(
            [
                new EventRecord(
                    EventId:
                        0,
                    Timestamp:
                        timestamp,
                    Category:
                        EventCategory.Command,
                    Type:
                        EventTypes.TagWriteSucceeded,
                    Severity:
                        EventSeverity.Information,
                    Source:
                        "plc01.command",
                    Message:
                        "Команда выполнена.",
                    DataJson:
                        """{"value":1}""",
                    ActorUserId:
                        "user-01",
                    ActorUserName:
                        "operator.one")
            ]);

        var events =
            await store.LoadAllEventsAsync();

        Assert.AreEqual(
            1,
            events.Count);

        var record =
            events[0];

        Assert.IsTrue(
            record.EventId > 0);
        Assert.AreEqual(
            timestamp.ToUniversalTime(),
            record.Timestamp);
        Assert.AreEqual(
            TimeSpan.Zero,
            record.Timestamp.Offset);
        Assert.AreEqual(
            EventCategory.Command,
            record.Category);
        Assert.AreEqual(
            EventTypes.TagWriteSucceeded,
            record.Type);
        Assert.AreEqual(
            EventSeverity.Information,
            record.Severity);
        Assert.AreEqual(
            "plc01.command",
            record.Source);
        Assert.AreEqual(
            "Команда выполнена.",
            record.Message);
        Assert.AreEqual(
            """{"value":1}""",
            record.DataJson);
        Assert.AreEqual(
            "user-01",
            record.ActorUserId);
        Assert.AreEqual(
            "operator.one",
            record.ActorUserName);
    }

    private static async Task<int> ReadSchemaVersionAsync(
        string databasePath)
    {
        await using var connection =
            new SqliteConnection(
                CreateConnectionString(
                    databasePath));

        await connection.OpenAsync();

        await using var command =
            connection.CreateCommand();

        command.CommandText =
            "PRAGMA user_version;";

        return Convert.ToInt32(
            await command.ExecuteScalarAsync());
    }

    private static string CreateConnectionString(
        string databasePath)
    {
        return new SqliteConnectionStringBuilder
        {
            DataSource = databasePath,
            Pooling = false
        }
        .ToString();
    }

    private sealed class TemporaryOperationalDatabase : IDisposable
    {
        private readonly string _directory;

        private TemporaryOperationalDatabase(
            string directory,
            string databasePath)
        {
            _directory =
                directory;
            DatabasePath =
                databasePath;
        }

        public string DatabasePath { get; }

        public static TemporaryOperationalDatabase Create()
        {
            var directory =
                Path.Combine(
                    Path.GetTempPath(),
                    "dispatcher-historian-tests",
                    Guid.NewGuid().ToString(
                        "N"));

            Directory.CreateDirectory(
                directory);

            return new TemporaryOperationalDatabase(
                directory,
                Path.Combine(
                    directory,
                    "operational.db"));
        }

        public void Dispose()
        {
            if (Directory.Exists(
                    _directory))
            {
                Directory.Delete(
                    _directory,
                    recursive: true);
            }
        }
    }
}
