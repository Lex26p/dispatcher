using Dispatcher.Server.Configuration;
using Microsoft.Data.Sqlite;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class SqliteConfigurationStoreTests
{
    [TestMethod]
    public async Task ReplaceAsync_PersistsDevicesAndTags_AcrossStoreInstances()
    {
        var device =
            new ModbusDeviceConfiguration(
                DeviceId: "plc01",
                Name: "PLC 01",
                Enabled: true,
                Host: "192.168.1.10",
                Port: 1502,
                UnitId: 7,
                PollIntervalMilliseconds: 250,
                RequestTimeoutMilliseconds: 900,
                Tags:
                [
                    new ModbusTagConfiguration(
                        TagId: "plc01.temperature",
                        Name: "Temperature",
                        Address: 100,
                        Writable: false),
                    new ModbusTagConfiguration(
                        TagId: "plc01.setpoint",
                        Name: "Setpoint",
                        Address: 101,
                        Writable: true)
                ]);

        using var database =
            await TestConfigurationDatabase.CreateAsync(
                device);

        var reopenedStore =
            new SqliteConfigurationStore(
                database.DatabasePath);

        await reopenedStore.InitializeAsync();

        var loaded =
            await reopenedStore.LoadAsync();

        Assert.AreEqual(1, loaded.Count);

        var loadedDevice =
            loaded[0];

        Assert.AreEqual("plc01", loadedDevice.DeviceId);
        Assert.AreEqual("PLC 01", loadedDevice.Name);
        Assert.IsTrue(loadedDevice.Enabled);
        Assert.AreEqual("192.168.1.10", loadedDevice.Host);
        Assert.AreEqual(1502, loadedDevice.Port);
        Assert.AreEqual(7, loadedDevice.UnitId);
        Assert.AreEqual(250, loadedDevice.PollIntervalMilliseconds);
        Assert.AreEqual(900, loadedDevice.RequestTimeoutMilliseconds);
        Assert.AreEqual(2, loadedDevice.Tags.Count);

        var setpoint =
            loadedDevice.Tags.Single(
                tag =>
                    tag.TagId
                    == "plc01.setpoint");

        Assert.AreEqual("Setpoint", setpoint.Name);
        Assert.AreEqual(101, setpoint.Address);
        Assert.IsTrue(setpoint.Writable);
    }

    [TestMethod]
    public async Task ReplaceSnmpAsync_PersistsSnmpDevicesAndTags_AcrossStoreInstances()
    {
        var device =
            new SnmpDeviceConfiguration(
                DeviceId: "switch01",
                Name: "Switch 01",
                Enabled: true,
                Host: "192.168.1.20",
                Port: 1161,
                Community: "monitoring",
                PollIntervalMilliseconds: 5000,
                RequestTimeoutMilliseconds: 1500,
                Tags:
                [
                    new SnmpTagConfiguration(
                        TagId: "switch01.sysName",
                        Name: "sysName",
                        Oid: "1.3.6.1.2.1.1.5.0")
                ]);

        using var database =
            await TestConfigurationDatabase.CreateAsync(
                Array.Empty<ModbusDeviceConfiguration>(),
                [device]);

        var reopenedStore =
            new SqliteConfigurationStore(
                database.DatabasePath);

        await reopenedStore.InitializeAsync();

        var loaded =
            await reopenedStore.LoadSnmpAsync();

        Assert.AreEqual(1, loaded.Count);

        var loadedDevice =
            loaded[0];

        Assert.AreEqual("switch01", loadedDevice.DeviceId);
        Assert.AreEqual("Switch 01", loadedDevice.Name);
        Assert.IsTrue(loadedDevice.Enabled);
        Assert.AreEqual("192.168.1.20", loadedDevice.Host);
        Assert.AreEqual(1161, loadedDevice.Port);
        Assert.AreEqual("monitoring", loadedDevice.Community);
        Assert.AreEqual(5000, loadedDevice.PollIntervalMilliseconds);
        Assert.AreEqual(1500, loadedDevice.RequestTimeoutMilliseconds);
        Assert.AreEqual(1, loadedDevice.Tags.Count);
        Assert.AreEqual(
            "1.3.6.1.2.1.1.5.0",
            loadedDevice.Tags[0].Oid);
    }

    [TestMethod]
    public async Task InitializeAsync_MigratesVersion1Database_ToVersion6WithoutLosingModbusData()
    {
        var directory =
            Path.Combine(
                Path.GetTempPath(),
                "dispatcher-tests",
                Guid.NewGuid().ToString("N"));

        Directory.CreateDirectory(
            directory);

        var databasePath =
            Path.Combine(
                directory,
                "dispatcher-v1.db");

        try
        {
            var connectionString =
                new SqliteConnectionStringBuilder
                {
                    DataSource = databasePath,
                    Pooling = false
                }
                .ToString();

            await using (var connection =
                new SqliteConnection(
                    connectionString))
            {
                await connection.OpenAsync();

                await using var command =
                    connection.CreateCommand();

                command.CommandText =
                    """
                    CREATE TABLE modbus_devices (
                        device_id TEXT NOT NULL PRIMARY KEY,
                        name TEXT NOT NULL,
                        enabled INTEGER NOT NULL,
                        host TEXT NOT NULL,
                        port INTEGER NOT NULL,
                        unit_id INTEGER NOT NULL,
                        poll_interval_ms INTEGER NOT NULL,
                        request_timeout_ms INTEGER NOT NULL
                    );

                    CREATE TABLE modbus_tags (
                        tag_id TEXT NOT NULL PRIMARY KEY,
                        device_id TEXT NOT NULL,
                        name TEXT NOT NULL,
                        address INTEGER NOT NULL,
                        writable INTEGER NOT NULL
                    );

                    INSERT INTO modbus_devices (
                        device_id,
                        name,
                        enabled,
                        host,
                        port,
                        unit_id,
                        poll_interval_ms,
                        request_timeout_ms)
                    VALUES (
                        'plc01',
                        'PLC 01',
                        0,
                        '127.0.0.1',
                        502,
                        1,
                        1000,
                        1000);

                    INSERT INTO modbus_tags (
                        tag_id,
                        device_id,
                        name,
                        address,
                        writable)
                    VALUES (
                        'plc01.hr0',
                        'plc01',
                        'HR 0',
                        0,
                        0);

                    PRAGMA user_version = 1;
                    """;

                await command.ExecuteNonQueryAsync();
            }

            var store =
                new SqliteConfigurationStore(
                    databasePath);

            await store.InitializeAsync();

            var modbus =
                await store.LoadAsync();
            var snmp =
                await store.LoadSnmpAsync();
            var historianPolicies =
                await store.LoadHistorianPoliciesAsync();
            var localUsers =
                await store.LoadLocalUsersAsync();

            Assert.AreEqual(1, modbus.Count);
            Assert.AreEqual(
                "plc01.hr0",
                modbus[0].Tags[0].TagId);
            Assert.AreEqual(0, snmp.Count);
            Assert.AreEqual(0, historianPolicies.Count);
            Assert.AreEqual(0, localUsers.Count);

            await using var verify =
                new SqliteConnection(
                    connectionString);

            await verify.OpenAsync();

            await using var versionCommand =
                verify.CreateCommand();

            versionCommand.CommandText =
                "PRAGMA user_version;";

            var version =
                Convert.ToInt32(
                    await versionCommand.ExecuteScalarAsync());

            Assert.AreEqual(6, version);
        }
        finally
        {
            if (Directory.Exists(
                    directory))
            {
                Directory.Delete(
                    directory,
                    recursive: true);
            }
        }
    }

    [TestMethod]
    public async Task InitializeAsync_MigratesVersion3Database_ToVersion6WithoutLosingMimics()
    {
        var directory =
            Path.Combine(
                Path.GetTempPath(),
                "dispatcher-tests",
                Guid.NewGuid().ToString("N"));

        Directory.CreateDirectory(
            directory);

        var databasePath =
            Path.Combine(
                directory,
                "dispatcher-v3.db");

        var connectionString =
            new SqliteConnectionStringBuilder
            {
                DataSource = databasePath,
                Pooling = false
            }
            .ToString();

        try
        {
            await using (var connection =
                new SqliteConnection(
                    connectionString))
            {
                await connection.OpenAsync();

                await using var command =
                    connection.CreateCommand();

                command.CommandText =
                    """
                    CREATE TABLE mimics (
                        mimic_id TEXT NOT NULL PRIMARY KEY,
                        name TEXT NOT NULL,
                        width INTEGER NOT NULL CHECK (width > 0),
                        height INTEGER NOT NULL CHECK (height > 0),
                        elements_json TEXT NOT NULL
                    );

                    INSERT INTO mimics (
                        mimic_id,
                        name,
                        width,
                        height,
                        elements_json)
                    VALUES (
                        'main',
                        'Main',
                        800,
                        450,
                        '[]');

                    PRAGMA user_version = 3;
                    """;

                await command.ExecuteNonQueryAsync();
            }

            var store =
                new SqliteConfigurationStore(
                    databasePath);

            await store.InitializeAsync();

            var mimics =
                await store.LoadMimicsAsync();
            var policies =
                await store.LoadHistorianPoliciesAsync();
            var localUsers =
                await store.LoadLocalUsersAsync();

            Assert.AreEqual(
                1,
                mimics.Count);
            Assert.AreEqual(
                "main",
                mimics[0].MimicId);
            Assert.AreEqual(
                0,
                policies.Count);
            Assert.AreEqual(
                0,
                localUsers.Count);

            await using var verify =
                new SqliteConnection(
                    connectionString);

            await verify.OpenAsync();

            await using var versionCommand =
                verify.CreateCommand();

            versionCommand.CommandText =
                "PRAGMA user_version;";

            var version =
                Convert.ToInt32(
                    await versionCommand.ExecuteScalarAsync());

            Assert.AreEqual(
                6,
                version);
        }
        finally
        {
            if (Directory.Exists(
                    directory))
            {
                Directory.Delete(
                    directory,
                    recursive: true);
            }
        }
    }
}
