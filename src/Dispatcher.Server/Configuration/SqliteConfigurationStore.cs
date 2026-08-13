using Microsoft.Data.Sqlite;

namespace Dispatcher.Server.Configuration;

public sealed class SqliteConfigurationStore
{
    private const int CurrentSchemaVersion = 1;

    private readonly string _connectionString;

    public SqliteConfigurationStore(
        string databasePath)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(databasePath);

        DatabasePath = Path.GetFullPath(databasePath);

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
            Path.GetDirectoryName(DatabasePath);

        if (!string.IsNullOrWhiteSpace(directory))
        {
            Directory.CreateDirectory(directory);
        }

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        var schemaVersion =
            await GetSchemaVersionAsync(
                connection,
                cancellationToken);

        if (schemaVersion == 0)
        {
            await CreateSchemaAsync(
                connection,
                cancellationToken);

            return;
        }

        if (schemaVersion != CurrentSchemaVersion)
        {
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

        var deviceRows = new List<DeviceRow>();

        await using (var command = connection.CreateCommand())
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

            while (await reader.ReadAsync(cancellationToken))
            {
                deviceRows.Add(
                    new DeviceRow(
                        DeviceId: reader.GetString(0),
                        Name: reader.GetString(1),
                        Enabled: reader.GetInt64(2) != 0,
                        Host: reader.GetString(3),
                        Port: reader.GetInt32(4),
                        UnitId: reader.GetInt32(5),
                        PollIntervalMilliseconds: reader.GetInt32(6),
                        RequestTimeoutMilliseconds: reader.GetInt32(7)));
            }
        }

        var tagsByDevice =
            deviceRows.ToDictionary(
                device => device.DeviceId,
                _ => new List<ModbusTagConfiguration>(),
                StringComparer.Ordinal);

        await using (var command = connection.CreateCommand())
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

            while (await reader.ReadAsync(cancellationToken))
            {
                var tagId = reader.GetString(0);
                var deviceId = reader.GetString(1);

                if (!tagsByDevice.TryGetValue(
                        deviceId,
                        out var tags))
                {
                    throw new InvalidOperationException(
                        $"Tag '{tagId}' references unknown device '{deviceId}'.");
                }

                tags.Add(
                    new ModbusTagConfiguration(
                        TagId: tagId,
                        Name: reader.GetString(2),
                        Address: reader.GetInt32(3),
                        Writable: reader.GetInt64(4) != 0));
            }
        }

        var devices = deviceRows
            .Select(device =>
                new ModbusDeviceConfiguration(
                    DeviceId: device.DeviceId,
                    Name: device.Name,
                    Enabled: device.Enabled,
                    Host: device.Host,
                    Port: device.Port,
                    UnitId: device.UnitId,
                    PollIntervalMilliseconds:
                        device.PollIntervalMilliseconds,
                    RequestTimeoutMilliseconds:
                        device.RequestTimeoutMilliseconds,
                    Tags: tagsByDevice[device.DeviceId].ToArray()))
            .ToArray();

        ModbusConfigurationValidator.Validate(devices);

        return devices;
    }

    public async Task ReplaceAsync(
        IReadOnlyCollection<ModbusDeviceConfiguration> devices,
        CancellationToken cancellationToken = default)
    {
        ModbusConfigurationValidator.Validate(devices);

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        using var transaction =
            connection.BeginTransaction();

        await using (var deleteTags = connection.CreateCommand())
        {
            deleteTags.Transaction = transaction;
            deleteTags.CommandText =
                "DELETE FROM modbus_tags;";

            await deleteTags.ExecuteNonQueryAsync(
                cancellationToken);
        }

        await using (var deleteDevices = connection.CreateCommand())
        {
            deleteDevices.Transaction = transaction;
            deleteDevices.CommandText =
                "DELETE FROM modbus_devices;";

            await deleteDevices.ExecuteNonQueryAsync(
                cancellationToken);
        }

        foreach (var device in devices)
        {
            await InsertDeviceAsync(
                connection,
                transaction,
                device,
                cancellationToken);

            foreach (var tag in device.Tags)
            {
                await InsertTagAsync(
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

        return Convert.ToInt32(result);
    }

    private static async Task CreateSchemaAsync(
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

            PRAGMA user_version = 1;
            """;

        await command.ExecuteNonQueryAsync(
            cancellationToken);
    }

    private static async Task InsertDeviceAsync(
        SqliteConnection connection,
        SqliteTransaction transaction,
        ModbusDeviceConfiguration device,
        CancellationToken cancellationToken)
    {
        await using var command =
            connection.CreateCommand();

        command.Transaction = transaction;
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

    private static async Task InsertTagAsync(
        SqliteConnection connection,
        SqliteTransaction transaction,
        string deviceId,
        ModbusTagConfiguration tag,
        CancellationToken cancellationToken)
    {
        await using var command =
            connection.CreateCommand();

        command.Transaction = transaction;
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

    private sealed record DeviceRow(
        string DeviceId,
        string Name,
        bool Enabled,
        string Host,
        int Port,
        int UnitId,
        int PollIntervalMilliseconds,
        int RequestTimeoutMilliseconds);
}
