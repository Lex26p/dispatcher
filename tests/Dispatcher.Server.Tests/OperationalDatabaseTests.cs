using Dispatcher.Server.Historian;
using Microsoft.Data.Sqlite;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class OperationalDatabaseTests
{
    [TestMethod]
    public async Task InitializeAsync_CreatesIndependentSchemaVersion1()
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
            1,
            version);

        var samples =
            await store.LoadAllAsync();

        Assert.AreEqual(
            0,
            samples.Count);
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
