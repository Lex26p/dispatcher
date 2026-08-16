using Dispatcher.Server.Alarms;
using Dispatcher.Server.Configuration;
using Microsoft.Data.Sqlite;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class AlarmDefinitionStoreTests
{
    [TestMethod]
    public async Task InitializeAsync_CreatesSchemaVersion7_AndRoundTripsAlarmDefinitions()
    {
        using var database =
            TemporaryConfigurationDatabase.Create();
        var store =
            new SqliteConfigurationStore(
                database.DatabasePath);

        await store.InitializeAsync();
        await store.InitializeAsync();

        Assert.AreEqual(
            7,
            await ReadSchemaVersionAsync(
                database.DatabasePath));

        await store.InsertAlarmDefinitionAsync(
            new AlarmDefinitionConfiguration(
                AlarmId:
                    "temperature.high",
                Name:
                    "High temperature",
                Enabled:
                    true,
                TagId:
                    "process.temperature",
                Condition:
                    AlarmCondition.High,
                Threshold:
                    80.125m,
                Severity:
                    AlarmSeverity.Warning,
                Message:
                    "Temperature is high.",
                DelayMilliseconds:
                    1500,
                Hysteresis:
                    2.375m));

        await store.InsertAlarmDefinitionAsync(
            new AlarmDefinitionConfiguration(
                AlarmId:
                    "pump.running",
                Name:
                    "Pump running",
                Enabled:
                    false,
                TagId:
                    "pump.running",
                Condition:
                    AlarmCondition.DigitalTrue,
                Threshold:
                    null,
                Severity:
                    AlarmSeverity.Information,
                Message:
                    "Pump is running.",
                DelayMilliseconds:
                    0,
                Hysteresis:
                    null));

        var loaded =
            await store.LoadAlarmDefinitionsAsync();

        Assert.AreEqual(
            2,
            loaded.Count);

        var high =
            loaded.Single(definition =>
                definition.AlarmId
                == "temperature.high");

        Assert.IsTrue(
            high.Threshold.HasValue);
        Assert.AreEqual(
            80.125m,
            high.Threshold.Value);
        Assert.IsTrue(
            high.Hysteresis.HasValue);
        Assert.AreEqual(
            2.375m,
            high.Hysteresis.Value);
        Assert.AreEqual(
            AlarmSeverity.Warning,
            high.Severity);

        var digital =
            loaded.Single(definition =>
                definition.AlarmId
                == "pump.running");

        Assert.IsNull(
            digital.Threshold);
        Assert.IsNull(
            digital.Hysteresis);
    }

    [TestMethod]
    public async Task InitializeAsync_MigratesVersion6ToVersion7_WithoutRemovingExistingConfiguration()
    {
        using var database =
            TemporaryConfigurationDatabase.Create();

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
                CREATE TABLE legacy_marker (
                    marker TEXT NOT NULL
                );

                INSERT INTO legacy_marker(marker)
                VALUES ('preserved');

                PRAGMA user_version = 6;
                """;

            await command.ExecuteNonQueryAsync();
        }

        var store =
            new SqliteConfigurationStore(
                database.DatabasePath);

        await store.InitializeAsync();

        Assert.AreEqual(
            7,
            await ReadSchemaVersionAsync(
                database.DatabasePath));

        await using var verificationConnection =
            new SqliteConnection(
                CreateConnectionString(
                    database.DatabasePath));
        await verificationConnection.OpenAsync();

        await using var markerCommand =
            verificationConnection.CreateCommand();
        markerCommand.CommandText =
            "SELECT marker FROM legacy_marker;";

        Assert.AreEqual(
            "preserved",
            Convert.ToString(
                await markerCommand.ExecuteScalarAsync()));

        await using var tableCommand =
            verificationConnection.CreateCommand();
        tableCommand.CommandText =
            """
            SELECT COUNT(*)
            FROM sqlite_master
            WHERE type = 'table'
              AND name = 'alarm_definitions';
            """;

        Assert.AreEqual(
            1L,
            Convert.ToInt64(
                await tableCommand.ExecuteScalarAsync()));
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
            DataSource =
                databasePath,
            Pooling =
                false
        }
        .ToString();
    }

    private sealed class TemporaryConfigurationDatabase : IDisposable
    {
        private readonly string _directory;

        private TemporaryConfigurationDatabase(
            string directory,
            string databasePath)
        {
            _directory =
                directory;
            DatabasePath =
                databasePath;
        }

        public string DatabasePath { get; }

        public static TemporaryConfigurationDatabase Create()
        {
            var directory =
                Path.Combine(
                    Path.GetTempPath(),
                    "dispatcher-alarm-store-tests",
                    Guid.NewGuid().ToString(
                        "N"));

            Directory.CreateDirectory(
                directory);

            return new TemporaryConfigurationDatabase(
                directory,
                Path.Combine(
                    directory,
                    "dispatcher.db"));
        }

        public void Dispose()
        {
            if (Directory.Exists(
                    _directory))
            {
                Directory.Delete(
                    _directory,
                    recursive:
                        true);
            }
        }
    }
}
