using System.Net;
using System.Net.Http.Json;
using Dispatcher.Contracts.Mimics;
using Dispatcher.Server.Configuration;
using Microsoft.Data.Sqlite;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class MimicRuntimeTests
{
    [TestMethod]
    public async Task MimicConfigurationApi_PersistsListsLoadsAndDeletesDefinition()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        var definition =
            CreateDefinition(
                "main");

        var putResponse =
            await client.PutAsJsonAsync(
                "/api/configuration/mimics/main",
                definition);

        Assert.AreEqual(
            HttpStatusCode.OK,
            putResponse.StatusCode);

        var summaries =
            await client.GetFromJsonAsync<MimicSummaryDto[]>(
                "/api/mimics");

        Assert.IsNotNull(
            summaries);
        Assert.AreEqual(
            1,
            summaries.Length);
        Assert.AreEqual(
            "main",
            summaries[0].MimicId);
        Assert.AreEqual(
            5,
            summaries[0].ElementCount);

        var loaded =
            await client.GetFromJsonAsync<MimicDefinitionDto>(
                "/api/mimics/main");

        Assert.IsNotNull(
            loaded);
        Assert.AreEqual(
            "Main mimic",
            loaded.Name);
        Assert.AreEqual(
            400,
            loaded.Width);
        Assert.AreEqual(
            260,
            loaded.Height);
        Assert.AreEqual(
            5,
            loaded.Elements.Count);
        Assert.AreEqual(
            MimicElementTypeDto.Button,
            loaded.Elements[4].Type);
        Assert.AreEqual(
            (ushort)1,
            loaded.Elements[4].CommandValue!.Value);

        var reopenedStore =
            new SqliteConfigurationStore(
                database.DatabasePath);

        await reopenedStore.InitializeAsync();

        var persisted =
            await reopenedStore.LoadMimicsAsync();

        Assert.AreEqual(
            1,
            persisted.Count);
        Assert.AreEqual(
            "plc01.temperature",
            persisted[0].Elements[2].TagId);

        var deleteResponse =
            await client.DeleteAsync(
                "/api/configuration/mimics/main");

        Assert.AreEqual(
            HttpStatusCode.NoContent,
            deleteResponse.StatusCode);

        var missingResponse =
            await client.GetAsync(
                "/api/mimics/main");

        Assert.AreEqual(
            HttpStatusCode.NotFound,
            missingResponse.StatusCode);
    }

    [TestMethod]
    public async Task MimicConfigurationApi_ButtonWithoutTag_ReturnsBadRequest()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        var definition =
            new MimicDefinitionDto(
                "invalid",
                "Invalid",
                200,
                100,
                [
                    new MimicElementDto(
                        "button01",
                        MimicElementTypeDto.Button,
                        10,
                        10,
                        100,
                        30,
                        "Start",
                        TagId: null,
                        CommandValue: 1)
                ]);

        var response =
            await client.PutAsJsonAsync(
                "/api/configuration/mimics/invalid",
                definition);

        Assert.AreEqual(
            HttpStatusCode.BadRequest,
            response.StatusCode);
    }

    [TestMethod]
    public async Task MimicConfigurationApi_PathIdMismatch_ReturnsBadRequest()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        var response =
            await client.PutAsJsonAsync(
                "/api/configuration/mimics/path-id",
                CreateDefinition(
                    "body-id"));

        Assert.AreEqual(
            HttpStatusCode.BadRequest,
            response.StatusCode);
    }

    [TestMethod]
    public async Task InitializeAsync_MigratesVersion2Database_ToVersion9WithoutLosingProtocolData()
    {
        var directory =
            Path.Combine(
                Path.GetTempPath(),
                "dispatcher-tests",
                Guid.NewGuid().ToString(
                    "N"));

        Directory.CreateDirectory(
            directory);

        var databasePath =
            Path.Combine(
                directory,
                "dispatcher-v2.db");

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
                    PRAGMA foreign_keys = ON;

                    CREATE TABLE modbus_devices (
                        device_id TEXT NOT NULL PRIMARY KEY,
                        name TEXT NOT NULL,
                        enabled INTEGER NOT NULL CHECK (enabled IN (0, 1)),
                        host TEXT NOT NULL,
                        port INTEGER NOT NULL CHECK (port BETWEEN 1 AND 65535),
                        unit_id INTEGER NOT NULL CHECK (unit_id BETWEEN 0 AND 255),
                        poll_interval_ms INTEGER NOT NULL CHECK (poll_interval_ms > 0),
                        request_timeout_ms INTEGER NOT NULL CHECK (request_timeout_ms > 0)
                    );

                    CREATE TABLE modbus_tags (
                        tag_id TEXT NOT NULL PRIMARY KEY,
                        device_id TEXT NOT NULL,
                        name TEXT NOT NULL,
                        address INTEGER NOT NULL CHECK (address BETWEEN 0 AND 65535),
                        writable INTEGER NOT NULL CHECK (writable IN (0, 1)),
                        FOREIGN KEY (device_id)
                            REFERENCES modbus_devices(device_id)
                            ON DELETE CASCADE
                    );

                    CREATE INDEX ix_modbus_tags_device_id
                        ON modbus_tags(device_id);

                    CREATE TABLE snmp_devices (
                        device_id TEXT NOT NULL PRIMARY KEY,
                        name TEXT NOT NULL,
                        enabled INTEGER NOT NULL CHECK (enabled IN (0, 1)),
                        host TEXT NOT NULL,
                        port INTEGER NOT NULL CHECK (port BETWEEN 1 AND 65535),
                        community TEXT NOT NULL,
                        poll_interval_ms INTEGER NOT NULL CHECK (poll_interval_ms > 0),
                        request_timeout_ms INTEGER NOT NULL CHECK (request_timeout_ms > 0)
                    );

                    CREATE TABLE snmp_tags (
                        tag_id TEXT NOT NULL PRIMARY KEY,
                        device_id TEXT NOT NULL,
                        name TEXT NOT NULL,
                        oid TEXT NOT NULL,
                        FOREIGN KEY (device_id)
                            REFERENCES snmp_devices(device_id)
                            ON DELETE CASCADE
                    );

                    CREATE INDEX ix_snmp_tags_device_id
                        ON snmp_tags(device_id);

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

                    PRAGMA user_version = 2;
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
            var mimics =
                await store.LoadMimicsAsync();
            var policies =
                await store.LoadHistorianPoliciesAsync();

            Assert.AreEqual(
                1,
                modbus.Count);
            Assert.AreEqual(
                "plc01.hr0",
                modbus[0].Tags[0].TagId);
            Assert.AreEqual(
                0,
                snmp.Count);
            Assert.AreEqual(
                0,
                mimics.Count);
            Assert.AreEqual(
                0,
                policies.Count);

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
                9,
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

    private static MimicDefinitionDto CreateDefinition(
        string mimicId)
    {
        return new MimicDefinitionDto(
            mimicId,
            "Main mimic",
            400,
            260,
            [
                new MimicElementDto(
                    "title",
                    MimicElementTypeDto.Text,
                    10,
                    10,
                    220,
                    30,
                    "Pump station",
                    TagId: null,
                    CommandValue: null),

                new MimicElementDto(
                    "frame",
                    MimicElementTypeDto.Rectangle,
                    10,
                    50,
                    300,
                    180,
                    Text: null,
                    TagId: null,
                    CommandValue: null),

                new MimicElementDto(
                    "temperature",
                    MimicElementTypeDto.Value,
                    30,
                    70,
                    160,
                    50,
                    "Temperature",
                    "plc01.temperature",
                    CommandValue: null),

                new MimicElementDto(
                    "running",
                    MimicElementTypeDto.Indicator,
                    30,
                    130,
                    160,
                    50,
                    "Running",
                    "plc01.running",
                    CommandValue: null),

                new MimicElementDto(
                    "start",
                    MimicElementTypeDto.Button,
                    210,
                    130,
                    80,
                    40,
                    "Start",
                    "plc01.setpoint",
                    CommandValue: 1)
            ]);
    }
}
