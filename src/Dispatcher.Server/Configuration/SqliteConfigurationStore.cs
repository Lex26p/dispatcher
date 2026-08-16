using System.Text.Json;
using Dispatcher.Server.Historian;
using Dispatcher.Server.Mimics;
using Dispatcher.Server.Security;
using Microsoft.Data.Sqlite;

namespace Dispatcher.Server.Configuration;

public sealed partial class SqliteConfigurationStore
{
    private const int CurrentSchemaVersion = 9;

    private readonly string _connectionString;

    public SqliteConfigurationStore(
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
                await CreateSchemaV5Async(
                    connection,
                    cancellationToken);

                await MigrateV5ToV6Async(
                    connection,
                    cancellationToken);

                await MigrateV6ToV7Async(
                    connection,
                    cancellationToken);

                await MigrateV7ToV8Async(
                    connection,
                    cancellationToken);

                await MigrateV8ToV9Async(
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

                await MigrateV3ToV4Async(
                    connection,
                    cancellationToken);

                await MigrateV4ToV5Async(
                    connection,
                    cancellationToken);

                await MigrateV5ToV6Async(
                    connection,
                    cancellationToken);

                await MigrateV6ToV7Async(
                    connection,
                    cancellationToken);

                await MigrateV7ToV8Async(
                    connection,
                    cancellationToken);

                await MigrateV8ToV9Async(
                    connection,
                    cancellationToken);
                return;

            case 2:
                await MigrateV2ToV3Async(
                    connection,
                    cancellationToken);

                await MigrateV3ToV4Async(
                    connection,
                    cancellationToken);

                await MigrateV4ToV5Async(
                    connection,
                    cancellationToken);

                await MigrateV5ToV6Async(
                    connection,
                    cancellationToken);

                await MigrateV6ToV7Async(
                    connection,
                    cancellationToken);

                await MigrateV7ToV8Async(
                    connection,
                    cancellationToken);

                await MigrateV8ToV9Async(
                    connection,
                    cancellationToken);
                return;

            case 3:
                await MigrateV3ToV4Async(
                    connection,
                    cancellationToken);

                await MigrateV4ToV5Async(
                    connection,
                    cancellationToken);

                await MigrateV5ToV6Async(
                    connection,
                    cancellationToken);

                await MigrateV6ToV7Async(
                    connection,
                    cancellationToken);

                await MigrateV7ToV8Async(
                    connection,
                    cancellationToken);

                await MigrateV8ToV9Async(
                    connection,
                    cancellationToken);
                return;

            case 4:
                await MigrateV4ToV5Async(
                    connection,
                    cancellationToken);

                await MigrateV5ToV6Async(
                    connection,
                    cancellationToken);

                await MigrateV6ToV7Async(
                    connection,
                    cancellationToken);

                await MigrateV7ToV8Async(
                    connection,
                    cancellationToken);

                await MigrateV8ToV9Async(
                    connection,
                    cancellationToken);
                return;

            case 5:
                await MigrateV5ToV6Async(
                    connection,
                    cancellationToken);

                await MigrateV6ToV7Async(
                    connection,
                    cancellationToken);

                await MigrateV7ToV8Async(
                    connection,
                    cancellationToken);

                await MigrateV8ToV9Async(
                    connection,
                    cancellationToken);
                return;

            case 6:
                await MigrateV6ToV7Async(
                    connection,
                    cancellationToken);

                await MigrateV7ToV8Async(
                    connection,
                    cancellationToken);

                await MigrateV8ToV9Async(
                    connection,
                    cancellationToken);
                return;

            case 7:
                await MigrateV7ToV8Async(
                    connection,
                    cancellationToken);

                await MigrateV8ToV9Async(
                    connection,
                    cancellationToken);
                return;

            case 8:
                await MigrateV8ToV9Async(
                    connection,
                    cancellationToken);
                return;

            case CurrentSchemaVersion:
                return;

            default:
                throw new InvalidOperationException(
                    $"Unsupported configuration database schema version {schemaVersion}. " +
                    $"Expected {CurrentSchemaVersion}.");
        }
    }

    public async Task<IReadOnlyList<ModbusDeviceConfiguration>> LoadAsync(
        CancellationToken cancellationToken = default)
    {
        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        var deviceRows =
            new List<ModbusDeviceRow>();

        await using (var command =
            connection.CreateCommand())
        {
            command.CommandText =
                """
                SELECT
                    device_id,
                    name,
                    enabled,
                    host,
                    port,
                    unit_id,
                    poll_interval_ms,
                    request_timeout_ms
                FROM modbus_devices
                ORDER BY device_id;
                """;

            await using var reader =
                await command.ExecuteReaderAsync(
                    cancellationToken);

            while (await reader.ReadAsync(
                cancellationToken))
            {
                deviceRows.Add(
                    new ModbusDeviceRow(
                        DeviceId:
                            reader.GetString(0),
                        Name:
                            reader.GetString(1),
                        Enabled:
                            reader.GetInt64(2) != 0,
                        Host:
                            reader.GetString(3),
                        Port:
                            reader.GetInt32(4),
                        UnitId:
                            reader.GetInt32(5),
                        PollIntervalMilliseconds:
                            reader.GetInt32(6),
                        RequestTimeoutMilliseconds:
                            reader.GetInt32(7)));
            }
        }

        var tagsByDevice =
            deviceRows.ToDictionary(
                device => device.DeviceId,
                _ => new List<ModbusTagConfiguration>(),
                StringComparer.Ordinal);

        await using (var command =
            connection.CreateCommand())
        {
            command.CommandText =
                """
                SELECT
                    tag_id,
                    device_id,
                    name,
                    address,
                    writable
                FROM modbus_tags
                ORDER BY device_id, tag_id;
                """;

            await using var reader =
                await command.ExecuteReaderAsync(
                    cancellationToken);

            while (await reader.ReadAsync(
                cancellationToken))
            {
                var tagId =
                    reader.GetString(0);
                var deviceId =
                    reader.GetString(1);

                if (!tagsByDevice.TryGetValue(
                        deviceId,
                        out var tags))
                {
                    throw new InvalidOperationException(
                        $"Tag '{tagId}' references unknown device '{deviceId}'.");
                }

                tags.Add(
                    new ModbusTagConfiguration(
                        TagId:
                            tagId,
                        Name:
                            reader.GetString(2),
                        Address:
                            reader.GetInt32(3),
                        Writable:
                            reader.GetInt64(4) != 0));
            }
        }

        var devices =
            deviceRows
                .Select(device =>
                    new ModbusDeviceConfiguration(
                        DeviceId:
                            device.DeviceId,
                        Name:
                            device.Name,
                        Enabled:
                            device.Enabled,
                        Host:
                            device.Host,
                        Port:
                            device.Port,
                        UnitId:
                            device.UnitId,
                        PollIntervalMilliseconds:
                            device.PollIntervalMilliseconds,
                        RequestTimeoutMilliseconds:
                            device.RequestTimeoutMilliseconds,
                        Tags:
                            tagsByDevice[
                                device.DeviceId]
                                .ToArray()))
                .ToArray();

        ModbusConfigurationValidator.Validate(
            devices);

        return devices;
    }

    public async Task<IReadOnlyList<SnmpDeviceConfiguration>> LoadSnmpAsync(
        CancellationToken cancellationToken = default)
    {
        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        var deviceRows =
            new List<SnmpDeviceRow>();

        await using (var command =
            connection.CreateCommand())
        {
            command.CommandText =
                """
                SELECT
                    device_id,
                    name,
                    enabled,
                    host,
                    port,
                    community,
                    poll_interval_ms,
                    request_timeout_ms
                FROM snmp_devices
                ORDER BY device_id;
                """;

            await using var reader =
                await command.ExecuteReaderAsync(
                    cancellationToken);

            while (await reader.ReadAsync(
                cancellationToken))
            {
                deviceRows.Add(
                    new SnmpDeviceRow(
                        DeviceId:
                            reader.GetString(0),
                        Name:
                            reader.GetString(1),
                        Enabled:
                            reader.GetInt64(2) != 0,
                        Host:
                            reader.GetString(3),
                        Port:
                            reader.GetInt32(4),
                        Community:
                            reader.GetString(5),
                        PollIntervalMilliseconds:
                            reader.GetInt32(6),
                        RequestTimeoutMilliseconds:
                            reader.GetInt32(7)));
            }
        }

        var tagsByDevice =
            deviceRows.ToDictionary(
                device => device.DeviceId,
                _ => new List<SnmpTagConfiguration>(),
                StringComparer.Ordinal);

        await using (var command =
            connection.CreateCommand())
        {
            command.CommandText =
                """
                SELECT
                    tag_id,
                    device_id,
                    name,
                    oid
                FROM snmp_tags
                ORDER BY device_id, tag_id;
                """;

            await using var reader =
                await command.ExecuteReaderAsync(
                    cancellationToken);

            while (await reader.ReadAsync(
                cancellationToken))
            {
                var tagId =
                    reader.GetString(0);
                var deviceId =
                    reader.GetString(1);

                if (!tagsByDevice.TryGetValue(
                        deviceId,
                        out var tags))
                {
                    throw new InvalidOperationException(
                        $"SNMP tag '{tagId}' references unknown device '{deviceId}'.");
                }

                tags.Add(
                    new SnmpTagConfiguration(
                        TagId:
                            tagId,
                        Name:
                            reader.GetString(2),
                        Oid:
                            reader.GetString(3)));
            }
        }

        var devices =
            deviceRows
                .Select(device =>
                    new SnmpDeviceConfiguration(
                        DeviceId:
                            device.DeviceId,
                        Name:
                            device.Name,
                        Enabled:
                            device.Enabled,
                        Host:
                            device.Host,
                        Port:
                            device.Port,
                        Community:
                            device.Community,
                        PollIntervalMilliseconds:
                            device.PollIntervalMilliseconds,
                        RequestTimeoutMilliseconds:
                            device.RequestTimeoutMilliseconds,
                        Tags:
                            tagsByDevice[
                                device.DeviceId]
                                .ToArray()))
                .ToArray();

        SnmpConfigurationValidator.Validate(
            devices);

        return devices;
    }

    public async Task<IReadOnlyList<HistorianPolicyConfiguration>> LoadHistorianPoliciesAsync(
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
                tag_id,
                enabled,
                mode,
                period_ms,
                retention_days
            FROM historian_policies
            ORDER BY tag_id;
            """;

        await using var reader =
            await command.ExecuteReaderAsync(
                cancellationToken);

        var policies =
            new List<HistorianPolicyConfiguration>();

        while (await reader.ReadAsync(
            cancellationToken))
        {
            policies.Add(
                new HistorianPolicyConfiguration(
                    TagId:
                        reader.GetString(0),
                    Enabled:
                        reader.GetInt64(1) != 0,
                    Mode:
                        (HistorianSamplingMode)reader.GetInt32(2),
                    PeriodMilliseconds:
                        reader.IsDBNull(3)
                            ? null
                            : reader.GetInt32(3),
                    RetentionDays:
                        reader.GetInt32(4)));
        }

        HistorianPolicyValidator.Validate(
            policies);

        return policies;
    }

    public async Task UpsertHistorianPolicyAsync(
        HistorianPolicyConfiguration policy,
        CancellationToken cancellationToken = default)
    {
        HistorianPolicyValidator.Validate(
            policy);

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        await using var command =
            connection.CreateCommand();

        command.CommandText =
            """
            INSERT INTO historian_policies (
                tag_id,
                enabled,
                mode,
                period_ms,
                retention_days)
            VALUES (
                $tagId,
                $enabled,
                $mode,
                $periodMilliseconds,
                $retentionDays)
            ON CONFLICT(tag_id) DO UPDATE SET
                enabled = excluded.enabled,
                mode = excluded.mode,
                period_ms = excluded.period_ms,
                retention_days = excluded.retention_days;
            """;

        command.Parameters.AddWithValue(
            "$tagId",
            policy.TagId);

        command.Parameters.AddWithValue(
            "$enabled",
            policy.Enabled ? 1 : 0);

        command.Parameters.AddWithValue(
            "$mode",
            (int)policy.Mode);

        command.Parameters.AddWithValue(
            "$periodMilliseconds",
            policy.PeriodMilliseconds is null
                ? DBNull.Value
                : policy.PeriodMilliseconds.Value);

        command.Parameters.AddWithValue(
            "$retentionDays",
            policy.RetentionDays);

        await command.ExecuteNonQueryAsync(
            cancellationToken);
    }

    public async Task<bool> DeleteHistorianPolicyAsync(
        string tagId,
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
            DELETE FROM historian_policies
            WHERE tag_id = $tagId;
            """;

        command.Parameters.AddWithValue(
            "$tagId",
            tagId);

        return await command.ExecuteNonQueryAsync(
            cancellationToken) > 0;
    }

    public async Task<IReadOnlyList<LocalUserConfiguration>> LoadLocalUsersAsync(
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
                user_id,
                user_name,
                normalized_user_name,
                display_name,
                enabled,
                password_hash
            FROM local_users
            ORDER BY normalized_user_name;
            """;

        await using var reader =
            await command.ExecuteReaderAsync(
                cancellationToken);

        var users =
            new List<LocalUserConfiguration>();

        while (await reader.ReadAsync(
            cancellationToken))
        {
            var user =
                new LocalUserConfiguration(
                    UserId:
                        reader.GetString(0),
                    UserName:
                        reader.GetString(1),
                    NormalizedUserName:
                        reader.GetString(2),
                    DisplayName:
                        reader.GetString(3),
                    Enabled:
                        reader.GetInt64(4) != 0,
                    PasswordHash:
                        reader.GetString(5));

            LocalUserConfigurationValidator.Validate(
                user);

            users.Add(
                user);
        }

        return users;
    }

    public async Task<LocalUserConfiguration?> FindLocalUserByNormalizedUserNameAsync(
        string normalizedUserName,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            normalizedUserName);

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        await using var command =
            connection.CreateCommand();

        command.CommandText =
            """
            SELECT
                user_id,
                user_name,
                normalized_user_name,
                display_name,
                enabled,
                password_hash
            FROM local_users
            WHERE normalized_user_name = $normalizedUserName;
            """;

        command.Parameters.AddWithValue(
            "$normalizedUserName",
            normalizedUserName);

        await using var reader =
            await command.ExecuteReaderAsync(
                cancellationToken);

        if (!await reader.ReadAsync(
                cancellationToken))
        {
            return null;
        }

        var user =
            new LocalUserConfiguration(
                UserId:
                    reader.GetString(0),
                UserName:
                    reader.GetString(1),
                NormalizedUserName:
                    reader.GetString(2),
                DisplayName:
                    reader.GetString(3),
                Enabled:
                    reader.GetInt64(4) != 0,
                PasswordHash:
                    reader.GetString(5));

        LocalUserConfigurationValidator.Validate(
            user);

        return user;
    }

    public async Task InsertLocalUserAsync(
        LocalUserConfiguration user,
        CancellationToken cancellationToken = default)
    {
        LocalUserConfigurationValidator.Validate(
            user);

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        await using var command =
            connection.CreateCommand();

        command.CommandText =
            """
            INSERT INTO local_users (
                user_id,
                user_name,
                normalized_user_name,
                display_name,
                enabled,
                password_hash)
            VALUES (
                $userId,
                $userName,
                $normalizedUserName,
                $displayName,
                $enabled,
                $passwordHash);
            """;

        command.Parameters.AddWithValue(
            "$userId",
            user.UserId);
        command.Parameters.AddWithValue(
            "$userName",
            user.UserName);
        command.Parameters.AddWithValue(
            "$normalizedUserName",
            user.NormalizedUserName);
        command.Parameters.AddWithValue(
            "$displayName",
            user.DisplayName);
        command.Parameters.AddWithValue(
            "$enabled",
            user.Enabled ? 1 : 0);
        command.Parameters.AddWithValue(
            "$passwordHash",
            user.PasswordHash);

        await command.ExecuteNonQueryAsync(
            cancellationToken);
    }

    public async Task<IReadOnlyList<MimicConfiguration>> LoadMimicsAsync(
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
                mimic_id,
                name,
                width,
                height,
                elements_json
            FROM mimics
            ORDER BY mimic_id;
            """;

        await using var reader =
            await command.ExecuteReaderAsync(
                cancellationToken);

        var mimics =
            new List<MimicConfiguration>();

        while (await reader.ReadAsync(
            cancellationToken))
        {
            var elements =
                JsonSerializer.Deserialize<MimicElementConfiguration[]>(
                    reader.GetString(4))
                ?? [];

            var mimic =
                new MimicConfiguration(
                    MimicId:
                        reader.GetString(0),
                    Name:
                        reader.GetString(1),
                    Width:
                        reader.GetInt32(2),
                    Height:
                        reader.GetInt32(3),
                    Elements:
                        elements);

            MimicConfigurationValidator.Validate(
                mimic);

            mimics.Add(
                mimic);
        }

        return mimics;
    }

    public async Task UpsertMimicAsync(
        MimicConfiguration mimic,
        CancellationToken cancellationToken = default)
    {
        MimicConfigurationValidator.Validate(
            mimic);

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        await using var command =
            connection.CreateCommand();

        command.CommandText =
            """
            INSERT INTO mimics (
                mimic_id,
                name,
                width,
                height,
                elements_json)
            VALUES (
                $mimicId,
                $name,
                $width,
                $height,
                $elementsJson)
            ON CONFLICT(mimic_id) DO UPDATE SET
                name = excluded.name,
                width = excluded.width,
                height = excluded.height,
                elements_json = excluded.elements_json;
            """;

        command.Parameters.AddWithValue(
            "$mimicId",
            mimic.MimicId);
        command.Parameters.AddWithValue(
            "$name",
            mimic.Name);
        command.Parameters.AddWithValue(
            "$width",
            mimic.Width);
        command.Parameters.AddWithValue(
            "$height",
            mimic.Height);
        command.Parameters.AddWithValue(
            "$elementsJson",
            JsonSerializer.Serialize(
                mimic.Elements));

        await command.ExecuteNonQueryAsync(
            cancellationToken);
    }

    public async Task<bool> DeleteMimicAsync(
        string mimicId,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            mimicId);

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        await using var command =
            connection.CreateCommand();

        command.CommandText =
            """
            DELETE FROM mimics
            WHERE mimic_id = $mimicId;
            """;

        command.Parameters.AddWithValue(
            "$mimicId",
            mimicId);

        return await command.ExecuteNonQueryAsync(
            cancellationToken) > 0;
    }

    public async Task ReplaceAsync(
        IReadOnlyCollection<ModbusDeviceConfiguration> devices,
        CancellationToken cancellationToken = default)
    {
        ModbusConfigurationValidator.Validate(
            devices);

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        using var transaction =
            connection.BeginTransaction();

        await DeleteProtocolConfigurationAsync(
            connection,
            transaction,
            "modbus_tags",
            "modbus_devices",
            cancellationToken);

        foreach (var device in devices)
        {
            await InsertModbusDeviceAsync(
                connection,
                transaction,
                device,
                cancellationToken);

            foreach (var tag in device.Tags)
            {
                await InsertModbusTagAsync(
                    connection,
                    transaction,
                    device.DeviceId,
                    tag,
                    cancellationToken);
            }
        }

        transaction.Commit();
    }

    public async Task ReplaceSnmpAsync(
        IReadOnlyCollection<SnmpDeviceConfiguration> devices,
        CancellationToken cancellationToken = default)
    {
        SnmpConfigurationValidator.Validate(
            devices);

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        using var transaction =
            connection.BeginTransaction();

        await DeleteProtocolConfigurationAsync(
            connection,
            transaction,
            "snmp_tags",
            "snmp_devices",
            cancellationToken);

        foreach (var device in devices)
        {
            await InsertSnmpDeviceAsync(
                connection,
                transaction,
                device,
                cancellationToken);

            foreach (var tag in device.Tags)
            {
                await InsertSnmpTagAsync(
                    connection,
                    transaction,
                    device.DeviceId,
                    tag,
                    cancellationToken);
            }
        }

        transaction.Commit();
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
            "PRAGMA foreign_keys = ON;";

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

    private static async Task CreateSchemaV5Async(
        SqliteConnection connection,
        CancellationToken cancellationToken)
    {
        await using var command =
            connection.CreateCommand();

        command.CommandText =
            """
            CREATE TABLE IF NOT EXISTS modbus_devices (
                device_id TEXT NOT NULL PRIMARY KEY,
                name TEXT NOT NULL,
                enabled INTEGER NOT NULL CHECK (enabled IN (0, 1)),
                host TEXT NOT NULL,
                port INTEGER NOT NULL CHECK (port BETWEEN 1 AND 65535),
                unit_id INTEGER NOT NULL CHECK (unit_id BETWEEN 0 AND 255),
                poll_interval_ms INTEGER NOT NULL CHECK (poll_interval_ms > 0),
                request_timeout_ms INTEGER NOT NULL CHECK (request_timeout_ms > 0)
            );

            CREATE TABLE IF NOT EXISTS modbus_tags (
                tag_id TEXT NOT NULL PRIMARY KEY,
                device_id TEXT NOT NULL,
                name TEXT NOT NULL,
                address INTEGER NOT NULL CHECK (address BETWEEN 0 AND 65535),
                writable INTEGER NOT NULL CHECK (writable IN (0, 1)),
                FOREIGN KEY (device_id)
                    REFERENCES modbus_devices(device_id)
                    ON DELETE CASCADE
            );

            CREATE INDEX IF NOT EXISTS ix_modbus_tags_device_id
                ON modbus_tags(device_id);

            CREATE TABLE IF NOT EXISTS snmp_devices (
                device_id TEXT NOT NULL PRIMARY KEY,
                name TEXT NOT NULL,
                enabled INTEGER NOT NULL CHECK (enabled IN (0, 1)),
                host TEXT NOT NULL,
                port INTEGER NOT NULL CHECK (port BETWEEN 1 AND 65535),
                community TEXT NOT NULL,
                poll_interval_ms INTEGER NOT NULL CHECK (poll_interval_ms > 0),
                request_timeout_ms INTEGER NOT NULL CHECK (request_timeout_ms > 0)
            );

            CREATE TABLE IF NOT EXISTS snmp_tags (
                tag_id TEXT NOT NULL PRIMARY KEY,
                device_id TEXT NOT NULL,
                name TEXT NOT NULL,
                oid TEXT NOT NULL,
                FOREIGN KEY (device_id)
                    REFERENCES snmp_devices(device_id)
                    ON DELETE CASCADE
            );

            CREATE INDEX IF NOT EXISTS ix_snmp_tags_device_id
                ON snmp_tags(device_id);

            CREATE TABLE IF NOT EXISTS mimics (
                mimic_id TEXT NOT NULL PRIMARY KEY,
                name TEXT NOT NULL,
                width INTEGER NOT NULL CHECK (width > 0),
                height INTEGER NOT NULL CHECK (height > 0),
                elements_json TEXT NOT NULL
            );

            CREATE TABLE IF NOT EXISTS historian_policies (
                tag_id TEXT NOT NULL PRIMARY KEY,
                enabled INTEGER NOT NULL CHECK (enabled IN (0, 1)),
                mode INTEGER NOT NULL CHECK (mode IN (0, 1)),
                period_ms INTEGER NULL,
                retention_days INTEGER NOT NULL CHECK (retention_days BETWEEN 1 AND 36500),
                CHECK (
                    (mode = 0 AND period_ms IS NULL)
                    OR
                    (mode = 1 AND period_ms IS NOT NULL AND period_ms BETWEEN 100 AND 86400000)
                )
            );

            CREATE TABLE IF NOT EXISTS local_users (
                user_id TEXT NOT NULL PRIMARY KEY,
                user_name TEXT NOT NULL,
                normalized_user_name TEXT NOT NULL UNIQUE,
                display_name TEXT NOT NULL,
                enabled INTEGER NOT NULL CHECK (enabled IN (0, 1)),
                password_hash TEXT NOT NULL
            );

            PRAGMA user_version = 5;
            """;

        await command.ExecuteNonQueryAsync(
            cancellationToken);
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
            CREATE TABLE IF NOT EXISTS snmp_devices (
                device_id TEXT NOT NULL PRIMARY KEY,
                name TEXT NOT NULL,
                enabled INTEGER NOT NULL CHECK (enabled IN (0, 1)),
                host TEXT NOT NULL,
                port INTEGER NOT NULL CHECK (port BETWEEN 1 AND 65535),
                community TEXT NOT NULL,
                poll_interval_ms INTEGER NOT NULL CHECK (poll_interval_ms > 0),
                request_timeout_ms INTEGER NOT NULL CHECK (request_timeout_ms > 0)
            );

            CREATE TABLE IF NOT EXISTS snmp_tags (
                tag_id TEXT NOT NULL PRIMARY KEY,
                device_id TEXT NOT NULL,
                name TEXT NOT NULL,
                oid TEXT NOT NULL,
                FOREIGN KEY (device_id)
                    REFERENCES snmp_devices(device_id)
                    ON DELETE CASCADE
            );

            CREATE INDEX IF NOT EXISTS ix_snmp_tags_device_id
                ON snmp_tags(device_id);

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
            CREATE TABLE IF NOT EXISTS mimics (
                mimic_id TEXT NOT NULL PRIMARY KEY,
                name TEXT NOT NULL,
                width INTEGER NOT NULL CHECK (width > 0),
                height INTEGER NOT NULL CHECK (height > 0),
                elements_json TEXT NOT NULL
            );

            PRAGMA user_version = 3;
            """;

        await command.ExecuteNonQueryAsync(
            cancellationToken);

        transaction.Commit();
    }

    private static async Task MigrateV3ToV4Async(
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
            CREATE TABLE IF NOT EXISTS historian_policies (
                tag_id TEXT NOT NULL PRIMARY KEY,
                enabled INTEGER NOT NULL CHECK (enabled IN (0, 1)),
                mode INTEGER NOT NULL CHECK (mode IN (0, 1)),
                period_ms INTEGER NULL,
                retention_days INTEGER NOT NULL CHECK (retention_days BETWEEN 1 AND 36500),
                CHECK (
                    (mode = 0 AND period_ms IS NULL)
                    OR
                    (mode = 1 AND period_ms IS NOT NULL AND period_ms BETWEEN 100 AND 86400000)
                )
            );

            PRAGMA user_version = 4;
            """;

        await command.ExecuteNonQueryAsync(
            cancellationToken);

        transaction.Commit();
    }

    private static async Task MigrateV4ToV5Async(
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
            CREATE TABLE IF NOT EXISTS local_users (
                user_id TEXT NOT NULL PRIMARY KEY,
                user_name TEXT NOT NULL,
                normalized_user_name TEXT NOT NULL UNIQUE,
                display_name TEXT NOT NULL,
                enabled INTEGER NOT NULL CHECK (enabled IN (0, 1)),
                password_hash TEXT NOT NULL
            );

            PRAGMA user_version = 5;
            """;

        await command.ExecuteNonQueryAsync(
            cancellationToken);

        transaction.Commit();
    }

    private static async Task MigrateV5ToV6Async(
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
            CREATE TABLE IF NOT EXISTS security_roles (
                role_id TEXT NOT NULL PRIMARY KEY,
                name TEXT NOT NULL,
                normalized_name TEXT NOT NULL UNIQUE,
                built_in INTEGER NOT NULL CHECK (built_in IN (0, 1))
            );

            CREATE TABLE IF NOT EXISTS security_role_permissions (
                role_id TEXT NOT NULL,
                permission TEXT NOT NULL,
                PRIMARY KEY (role_id, permission),
                FOREIGN KEY (role_id)
                    REFERENCES security_roles(role_id)
                    ON DELETE CASCADE
            );

            CREATE TABLE IF NOT EXISTS security_user_roles (
                user_id TEXT NOT NULL,
                role_id TEXT NOT NULL,
                PRIMARY KEY (user_id, role_id),
                FOREIGN KEY (user_id)
                    REFERENCES local_users(user_id)
                    ON DELETE CASCADE,
                FOREIGN KEY (role_id)
                    REFERENCES security_roles(role_id)
                    ON DELETE CASCADE
            );

            CREATE INDEX IF NOT EXISTS ix_security_user_roles_role_id
                ON security_user_roles(role_id);

            PRAGMA user_version = 6;
            """;

        await command.ExecuteNonQueryAsync(
            cancellationToken);

        transaction.Commit();
    }

    private static async Task MigrateV6ToV7Async(
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
            CREATE TABLE IF NOT EXISTS alarm_definitions (
                alarm_id TEXT NOT NULL PRIMARY KEY,
                name TEXT NOT NULL,
                enabled INTEGER NOT NULL CHECK (enabled IN (0, 1)),
                tag_id TEXT NOT NULL,
                condition INTEGER NOT NULL CHECK (condition BETWEEN 0 AND 3),
                threshold_text TEXT NULL,
                severity INTEGER NOT NULL CHECK (severity BETWEEN 0 AND 2),
                message TEXT NOT NULL,
                delay_ms INTEGER NOT NULL CHECK (delay_ms >= 0),
                hysteresis_text TEXT NULL,
                CHECK (
                    (condition IN (0, 1)
                        AND threshold_text IS NULL
                        AND hysteresis_text IS NULL)
                    OR
                    (condition IN (2, 3)
                        AND threshold_text IS NOT NULL
                        AND hysteresis_text IS NOT NULL)
                )
            );

            CREATE INDEX IF NOT EXISTS ix_alarm_definitions_tag_id
                ON alarm_definitions(tag_id);

            PRAGMA user_version = 7;
            """;

        await command.ExecuteNonQueryAsync(
            cancellationToken);

        transaction.Commit();
    }

    private static async Task MigrateV7ToV8Async(
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
            CREATE TABLE IF NOT EXISTS mimic_templates (
                template_id TEXT NOT NULL PRIMARY KEY,
                name TEXT NOT NULL,
                width INTEGER NOT NULL CHECK (width > 0),
                height INTEGER NOT NULL CHECK (height > 0),
                parameters_json TEXT NOT NULL,
                elements_json TEXT NOT NULL
            );

            PRAGMA user_version = 8;
            """;

        await command.ExecuteNonQueryAsync(
            cancellationToken);

        transaction.Commit();
    }

    private static async Task MigrateV8ToV9Async(
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
            CREATE TABLE templates (
                template_id TEXT NOT NULL PRIMARY KEY,
                name TEXT NOT NULL,
                kind INTEGER NOT NULL CHECK (kind BETWEEN 0 AND 2),
                version INTEGER NOT NULL CHECK (version >= 1),
                parameters_json TEXT NOT NULL
            );

            INSERT INTO templates (
                template_id,
                name,
                kind,
                version,
                parameters_json)
            SELECT
                template_id,
                name,
                0,
                1,
                parameters_json
            FROM mimic_templates;

            CREATE TABLE mimic_templates_v9 (
                template_id TEXT NOT NULL PRIMARY KEY,
                width INTEGER NOT NULL CHECK (width > 0),
                height INTEGER NOT NULL CHECK (height > 0),
                elements_json TEXT NOT NULL,
                FOREIGN KEY (template_id)
                    REFERENCES templates(template_id)
                    ON DELETE CASCADE
            );

            INSERT INTO mimic_templates_v9 (
                template_id,
                width,
                height,
                elements_json)
            SELECT
                template_id,
                width,
                height,
                elements_json
            FROM mimic_templates;

            DROP TABLE mimic_templates;
            ALTER TABLE mimic_templates_v9 RENAME TO mimic_templates;

            CREATE TABLE modbus_device_templates (
                template_id TEXT NOT NULL PRIMARY KEY,
                device_name TEXT NOT NULL,
                device_name_parameter_id TEXT NULL,
                host_parameter_id TEXT NOT NULL,
                tag_id_prefix_parameter_id TEXT NOT NULL,
                enabled INTEGER NOT NULL CHECK (enabled IN (0, 1)),
                port INTEGER NOT NULL CHECK (port BETWEEN 1 AND 65535),
                unit_id INTEGER NOT NULL CHECK (unit_id BETWEEN 0 AND 255),
                poll_interval_ms INTEGER NOT NULL CHECK (poll_interval_ms > 0),
                request_timeout_ms INTEGER NOT NULL CHECK (request_timeout_ms > 0),
                tags_json TEXT NOT NULL,
                FOREIGN KEY (template_id)
                    REFERENCES templates(template_id)
                    ON DELETE CASCADE
            );

            CREATE TABLE snmp_device_templates (
                template_id TEXT NOT NULL PRIMARY KEY,
                device_name TEXT NOT NULL,
                device_name_parameter_id TEXT NULL,
                host_parameter_id TEXT NOT NULL,
                community_parameter_id TEXT NOT NULL,
                tag_id_prefix_parameter_id TEXT NOT NULL,
                enabled INTEGER NOT NULL CHECK (enabled IN (0, 1)),
                port INTEGER NOT NULL CHECK (port BETWEEN 1 AND 65535),
                poll_interval_ms INTEGER NOT NULL CHECK (poll_interval_ms > 0),
                request_timeout_ms INTEGER NOT NULL CHECK (request_timeout_ms > 0),
                tags_json TEXT NOT NULL,
                FOREIGN KEY (template_id)
                    REFERENCES templates(template_id)
                    ON DELETE CASCADE
            );

            PRAGMA user_version = 9;
            """;

        await command.ExecuteNonQueryAsync(
            cancellationToken);

        transaction.Commit();
    }

    private static async Task DeleteProtocolConfigurationAsync(
        SqliteConnection connection,
        SqliteTransaction transaction,
        string tagTable,
        string deviceTable,
        CancellationToken cancellationToken)
    {
        await using (var deleteTags =
            connection.CreateCommand())
        {
            deleteTags.Transaction =
                transaction;
            deleteTags.CommandText =
                $"DELETE FROM {tagTable};";

            await deleteTags.ExecuteNonQueryAsync(
                cancellationToken);
        }

        await using (var deleteDevices =
            connection.CreateCommand())
        {
            deleteDevices.Transaction =
                transaction;
            deleteDevices.CommandText =
                $"DELETE FROM {deviceTable};";

            await deleteDevices.ExecuteNonQueryAsync(
                cancellationToken);
        }
    }

    private static async Task InsertModbusDeviceAsync(
        SqliteConnection connection,
        SqliteTransaction transaction,
        ModbusDeviceConfiguration device,
        CancellationToken cancellationToken)
    {
        await using var command =
            connection.CreateCommand();

        command.Transaction =
            transaction;
        command.CommandText =
            """
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
                $deviceId,
                $name,
                $enabled,
                $host,
                $port,
                $unitId,
                $pollInterval,
                $requestTimeout);
            """;

        command.Parameters.AddWithValue(
            "$deviceId",
            device.DeviceId);
        command.Parameters.AddWithValue(
            "$name",
            device.Name);
        command.Parameters.AddWithValue(
            "$enabled",
            device.Enabled ? 1 : 0);
        command.Parameters.AddWithValue(
            "$host",
            device.Host);
        command.Parameters.AddWithValue(
            "$port",
            device.Port);
        command.Parameters.AddWithValue(
            "$unitId",
            device.UnitId);
        command.Parameters.AddWithValue(
            "$pollInterval",
            device.PollIntervalMilliseconds);
        command.Parameters.AddWithValue(
            "$requestTimeout",
            device.RequestTimeoutMilliseconds);

        await command.ExecuteNonQueryAsync(
            cancellationToken);
    }

    private static async Task InsertModbusTagAsync(
        SqliteConnection connection,
        SqliteTransaction transaction,
        string deviceId,
        ModbusTagConfiguration tag,
        CancellationToken cancellationToken)
    {
        await using var command =
            connection.CreateCommand();

        command.Transaction =
            transaction;
        command.CommandText =
            """
            INSERT INTO modbus_tags (
                tag_id,
                device_id,
                name,
                address,
                writable)
            VALUES (
                $tagId,
                $deviceId,
                $name,
                $address,
                $writable);
            """;

        command.Parameters.AddWithValue(
            "$tagId",
            tag.TagId);
        command.Parameters.AddWithValue(
            "$deviceId",
            deviceId);
        command.Parameters.AddWithValue(
            "$name",
            tag.Name);
        command.Parameters.AddWithValue(
            "$address",
            tag.Address);
        command.Parameters.AddWithValue(
            "$writable",
            tag.Writable ? 1 : 0);

        await command.ExecuteNonQueryAsync(
            cancellationToken);
    }

    private static async Task InsertSnmpDeviceAsync(
        SqliteConnection connection,
        SqliteTransaction transaction,
        SnmpDeviceConfiguration device,
        CancellationToken cancellationToken)
    {
        await using var command =
            connection.CreateCommand();

        command.Transaction =
            transaction;
        command.CommandText =
            """
            INSERT INTO snmp_devices (
                device_id,
                name,
                enabled,
                host,
                port,
                community,
                poll_interval_ms,
                request_timeout_ms)
            VALUES (
                $deviceId,
                $name,
                $enabled,
                $host,
                $port,
                $community,
                $pollInterval,
                $requestTimeout);
            """;

        command.Parameters.AddWithValue(
            "$deviceId",
            device.DeviceId);
        command.Parameters.AddWithValue(
            "$name",
            device.Name);
        command.Parameters.AddWithValue(
            "$enabled",
            device.Enabled ? 1 : 0);
        command.Parameters.AddWithValue(
            "$host",
            device.Host);
        command.Parameters.AddWithValue(
            "$port",
            device.Port);
        command.Parameters.AddWithValue(
            "$community",
            device.Community);
        command.Parameters.AddWithValue(
            "$pollInterval",
            device.PollIntervalMilliseconds);
        command.Parameters.AddWithValue(
            "$requestTimeout",
            device.RequestTimeoutMilliseconds);

        await command.ExecuteNonQueryAsync(
            cancellationToken);
    }

    private static async Task InsertSnmpTagAsync(
        SqliteConnection connection,
        SqliteTransaction transaction,
        string deviceId,
        SnmpTagConfiguration tag,
        CancellationToken cancellationToken)
    {
        await using var command =
            connection.CreateCommand();

        command.Transaction =
            transaction;
        command.CommandText =
            """
            INSERT INTO snmp_tags (
                tag_id,
                device_id,
                name,
                oid)
            VALUES (
                $tagId,
                $deviceId,
                $name,
                $oid);
            """;

        command.Parameters.AddWithValue(
            "$tagId",
            tag.TagId);
        command.Parameters.AddWithValue(
            "$deviceId",
            deviceId);
        command.Parameters.AddWithValue(
            "$name",
            tag.Name);
        command.Parameters.AddWithValue(
            "$oid",
            tag.Oid);

        await command.ExecuteNonQueryAsync(
            cancellationToken);
    }

    private sealed record ModbusDeviceRow(
        string DeviceId,
        string Name,
        bool Enabled,
        string Host,
        int Port,
        int UnitId,
        int PollIntervalMilliseconds,
        int RequestTimeoutMilliseconds);

    private sealed record SnmpDeviceRow(
        string DeviceId,
        string Name,
        bool Enabled,
        string Host,
        int Port,
        string Community,
        int PollIntervalMilliseconds,
        int RequestTimeoutMilliseconds);
}
