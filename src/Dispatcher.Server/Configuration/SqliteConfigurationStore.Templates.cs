using System.Text.Json;
using Dispatcher.Server.Templates;
using Microsoft.Data.Sqlite;

namespace Dispatcher.Server.Configuration;

public sealed partial class SqliteConfigurationStore
{
    public async Task<IReadOnlyList<TemplateCatalogEntryConfiguration>> LoadTemplateCatalogAsync(
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
                template_id,
                name,
                kind,
                version,
                parameters_json
            FROM templates
            ORDER BY kind, template_id;
            """;

        await using var reader =
            await command.ExecuteReaderAsync(
                cancellationToken);
        var entries =
            new List<TemplateCatalogEntryConfiguration>();

        while (await reader.ReadAsync(
            cancellationToken))
        {
            var entry =
                new TemplateCatalogEntryConfiguration(
                    reader.GetString(0),
                    reader.GetString(1),
                    (TemplateKind)reader.GetInt32(2),
                    reader.GetInt32(3),
                    JsonSerializer.Deserialize<TemplateParameterConfiguration[]>(
                        reader.GetString(4))
                    ?? []);

            TemplateConfigurationValidator.ValidateCatalogEntry(
                entry);
            entries.Add(
                entry);
        }

        return entries;
    }

    public async Task<IReadOnlyList<ModbusDeviceTemplateConfiguration>> LoadModbusDeviceTemplatesAsync(
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
                t.template_id,
                t.name,
                t.version,
                t.parameters_json,
                p.device_name,
                p.device_name_parameter_id,
                p.host_parameter_id,
                p.tag_id_prefix_parameter_id,
                p.enabled,
                p.port,
                p.unit_id,
                p.poll_interval_ms,
                p.request_timeout_ms,
                p.tags_json
            FROM templates t
            INNER JOIN modbus_device_templates p
                ON p.template_id = t.template_id
            WHERE t.kind = $kind
            ORDER BY t.template_id;
            """;
        command.Parameters.AddWithValue(
            "$kind",
            (int)TemplateKind.ModbusDevice);

        await using var reader =
            await command.ExecuteReaderAsync(
                cancellationToken);
        var templates =
            new List<ModbusDeviceTemplateConfiguration>();

        while (await reader.ReadAsync(
            cancellationToken))
        {
            var template =
                new ModbusDeviceTemplateConfiguration(
                    reader.GetString(0),
                    reader.GetString(1),
                    reader.GetInt32(2),
                    JsonSerializer.Deserialize<TemplateParameterConfiguration[]>(
                        reader.GetString(3))
                    ?? [],
                    reader.GetString(4),
                    reader.IsDBNull(5) ? null : reader.GetString(5),
                    reader.GetString(6),
                    reader.GetString(7),
                    reader.GetInt64(8) != 0,
                    reader.GetInt32(9),
                    reader.GetInt32(10),
                    reader.GetInt32(11),
                    reader.GetInt32(12),
                    JsonSerializer.Deserialize<ModbusTagTemplateConfiguration[]>(
                        reader.GetString(13))
                    ?? []);

            DeviceTemplateConfigurationValidator.Validate(
                template);
            templates.Add(
                template);
        }

        return templates;
    }

    public async Task<IReadOnlyList<SnmpDeviceTemplateConfiguration>> LoadSnmpDeviceTemplatesAsync(
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
                t.template_id,
                t.name,
                t.version,
                t.parameters_json,
                p.device_name,
                p.device_name_parameter_id,
                p.host_parameter_id,
                p.community_parameter_id,
                p.tag_id_prefix_parameter_id,
                p.enabled,
                p.port,
                p.poll_interval_ms,
                p.request_timeout_ms,
                p.tags_json
            FROM templates t
            INNER JOIN snmp_device_templates p
                ON p.template_id = t.template_id
            WHERE t.kind = $kind
            ORDER BY t.template_id;
            """;
        command.Parameters.AddWithValue(
            "$kind",
            (int)TemplateKind.SnmpDevice);

        await using var reader =
            await command.ExecuteReaderAsync(
                cancellationToken);
        var templates =
            new List<SnmpDeviceTemplateConfiguration>();

        while (await reader.ReadAsync(
            cancellationToken))
        {
            var template =
                new SnmpDeviceTemplateConfiguration(
                    reader.GetString(0),
                    reader.GetString(1),
                    reader.GetInt32(2),
                    JsonSerializer.Deserialize<TemplateParameterConfiguration[]>(
                        reader.GetString(3))
                    ?? [],
                    reader.GetString(4),
                    reader.IsDBNull(5) ? null : reader.GetString(5),
                    reader.GetString(6),
                    reader.GetString(7),
                    reader.GetString(8),
                    reader.GetInt64(9) != 0,
                    reader.GetInt32(10),
                    reader.GetInt32(11),
                    reader.GetInt32(12),
                    JsonSerializer.Deserialize<SnmpTagTemplateConfiguration[]>(
                        reader.GetString(13))
                    ?? []);

            DeviceTemplateConfigurationValidator.Validate(
                template);
            templates.Add(
                template);
        }

        return templates;
    }

    public async Task<ModbusDeviceTemplateConfiguration> UpsertModbusDeviceTemplateAsync(
        ModbusDeviceTemplateConfiguration template,
        CancellationToken cancellationToken = default)
    {
        DeviceTemplateConfigurationValidator.Validate(
            template);
        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);
        using var transaction =
            connection.BeginTransaction();

        var version =
            await UpsertTemplateCatalogEntryAsync(
                connection,
                transaction,
                template.TemplateId,
                template.Name,
                TemplateKind.ModbusDevice,
                template.Parameters,
                cancellationToken);

        await using var command =
            connection.CreateCommand();
        command.Transaction = transaction;
        command.CommandText =
            """
            INSERT INTO modbus_device_templates (
                template_id,
                device_name,
                device_name_parameter_id,
                host_parameter_id,
                tag_id_prefix_parameter_id,
                enabled,
                port,
                unit_id,
                poll_interval_ms,
                request_timeout_ms,
                tags_json)
            VALUES (
                $templateId,
                $deviceName,
                $deviceNameParameterId,
                $hostParameterId,
                $tagIdPrefixParameterId,
                $enabled,
                $port,
                $unitId,
                $pollInterval,
                $requestTimeout,
                $tagsJson)
            ON CONFLICT(template_id) DO UPDATE SET
                device_name = excluded.device_name,
                device_name_parameter_id = excluded.device_name_parameter_id,
                host_parameter_id = excluded.host_parameter_id,
                tag_id_prefix_parameter_id = excluded.tag_id_prefix_parameter_id,
                enabled = excluded.enabled,
                port = excluded.port,
                unit_id = excluded.unit_id,
                poll_interval_ms = excluded.poll_interval_ms,
                request_timeout_ms = excluded.request_timeout_ms,
                tags_json = excluded.tags_json;
            """;
        AddCommonDeviceTemplateParameters(
            command,
            template.TemplateId,
            template.DeviceName,
            template.DeviceNameParameterId,
            template.HostParameterId,
            template.TagIdPrefixParameterId,
            template.Enabled,
            template.Port,
            template.PollIntervalMilliseconds,
            template.RequestTimeoutMilliseconds);
        command.Parameters.AddWithValue(
            "$unitId",
            template.UnitId);
        command.Parameters.AddWithValue(
            "$tagsJson",
            JsonSerializer.Serialize(
                template.Tags));
        await command.ExecuteNonQueryAsync(
            cancellationToken);
        transaction.Commit();

        return template with
        {
            Version = version
        };
    }

    public async Task<SnmpDeviceTemplateConfiguration> UpsertSnmpDeviceTemplateAsync(
        SnmpDeviceTemplateConfiguration template,
        CancellationToken cancellationToken = default)
    {
        DeviceTemplateConfigurationValidator.Validate(
            template);
        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);
        using var transaction =
            connection.BeginTransaction();

        var version =
            await UpsertTemplateCatalogEntryAsync(
                connection,
                transaction,
                template.TemplateId,
                template.Name,
                TemplateKind.SnmpDevice,
                template.Parameters,
                cancellationToken);

        await using var command =
            connection.CreateCommand();
        command.Transaction = transaction;
        command.CommandText =
            """
            INSERT INTO snmp_device_templates (
                template_id,
                device_name,
                device_name_parameter_id,
                host_parameter_id,
                community_parameter_id,
                tag_id_prefix_parameter_id,
                enabled,
                port,
                poll_interval_ms,
                request_timeout_ms,
                tags_json)
            VALUES (
                $templateId,
                $deviceName,
                $deviceNameParameterId,
                $hostParameterId,
                $communityParameterId,
                $tagIdPrefixParameterId,
                $enabled,
                $port,
                $pollInterval,
                $requestTimeout,
                $tagsJson)
            ON CONFLICT(template_id) DO UPDATE SET
                device_name = excluded.device_name,
                device_name_parameter_id = excluded.device_name_parameter_id,
                host_parameter_id = excluded.host_parameter_id,
                community_parameter_id = excluded.community_parameter_id,
                tag_id_prefix_parameter_id = excluded.tag_id_prefix_parameter_id,
                enabled = excluded.enabled,
                port = excluded.port,
                poll_interval_ms = excluded.poll_interval_ms,
                request_timeout_ms = excluded.request_timeout_ms,
                tags_json = excluded.tags_json;
            """;
        AddCommonDeviceTemplateParameters(
            command,
            template.TemplateId,
            template.DeviceName,
            template.DeviceNameParameterId,
            template.HostParameterId,
            template.TagIdPrefixParameterId,
            template.Enabled,
            template.Port,
            template.PollIntervalMilliseconds,
            template.RequestTimeoutMilliseconds);
        command.Parameters.AddWithValue(
            "$communityParameterId",
            template.CommunityParameterId);
        command.Parameters.AddWithValue(
            "$tagsJson",
            JsonSerializer.Serialize(
                template.Tags));
        await command.ExecuteNonQueryAsync(
            cancellationToken);
        transaction.Commit();

        return template with
        {
            Version = version
        };
    }

    public async Task<bool> DeleteTemplateAsync(
        string templateId,
        TemplateKind kind,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            templateId);
        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);
        await using var command =
            connection.CreateCommand();
        command.CommandText =
            """
            DELETE FROM templates
            WHERE template_id = $templateId
              AND kind = $kind;
            """;
        command.Parameters.AddWithValue(
            "$templateId",
            templateId);
        command.Parameters.AddWithValue(
            "$kind",
            (int)kind);

        return await command.ExecuteNonQueryAsync(
            cancellationToken) > 0;
    }

    private static async Task<int> UpsertTemplateCatalogEntryAsync(
        SqliteConnection connection,
        SqliteTransaction transaction,
        string templateId,
        string name,
        TemplateKind kind,
        IReadOnlyList<TemplateParameterConfiguration> parameters,
        CancellationToken cancellationToken)
    {
        var existing =
            await FindTemplateIdentityAsync(
                connection,
                transaction,
                templateId,
                cancellationToken);

        if (existing is not null
            && existing.Value.Kind != kind)
        {
            throw new TemplateConflictException(
                $"TemplateId '{templateId}' already belongs to template kind '{existing.Value.Kind}'.");
        }

        var version =
            existing is null
                ? 1
                : checked(existing.Value.Version + 1);

        var entry =
            new TemplateCatalogEntryConfiguration(
                templateId,
                name,
                kind,
                version,
                parameters);
        TemplateConfigurationValidator.ValidateCatalogEntry(
            entry);

        await using var command =
            connection.CreateCommand();
        command.Transaction = transaction;
        command.CommandText =
            """
            INSERT INTO templates (
                template_id,
                name,
                kind,
                version,
                parameters_json)
            VALUES (
                $templateId,
                $name,
                $kind,
                $version,
                $parametersJson)
            ON CONFLICT(template_id) DO UPDATE SET
                name = excluded.name,
                version = excluded.version,
                parameters_json = excluded.parameters_json;
            """;
        command.Parameters.AddWithValue(
            "$templateId",
            templateId);
        command.Parameters.AddWithValue(
            "$name",
            name);
        command.Parameters.AddWithValue(
            "$kind",
            (int)kind);
        command.Parameters.AddWithValue(
            "$version",
            version);
        command.Parameters.AddWithValue(
            "$parametersJson",
            JsonSerializer.Serialize(
                parameters));
        await command.ExecuteNonQueryAsync(
            cancellationToken);

        return version;
    }

    private static async Task<(TemplateKind Kind, int Version)?> FindTemplateIdentityAsync(
        SqliteConnection connection,
        SqliteTransaction transaction,
        string templateId,
        CancellationToken cancellationToken)
    {
        await using var command =
            connection.CreateCommand();
        command.Transaction = transaction;
        command.CommandText =
            """
            SELECT kind, version
            FROM templates
            WHERE template_id = $templateId;
            """;
        command.Parameters.AddWithValue(
            "$templateId",
            templateId);
        await using var reader =
            await command.ExecuteReaderAsync(
                cancellationToken);

        if (!await reader.ReadAsync(
                cancellationToken))
        {
            return null;
        }

        return (
            (TemplateKind)reader.GetInt32(0),
            reader.GetInt32(1));
    }

    private static void AddCommonDeviceTemplateParameters(
        SqliteCommand command,
        string templateId,
        string deviceName,
        string? deviceNameParameterId,
        string hostParameterId,
        string tagIdPrefixParameterId,
        bool enabled,
        int port,
        int pollIntervalMilliseconds,
        int requestTimeoutMilliseconds)
    {
        command.Parameters.AddWithValue(
            "$templateId",
            templateId);
        command.Parameters.AddWithValue(
            "$deviceName",
            deviceName);
        command.Parameters.AddWithValue(
            "$deviceNameParameterId",
            deviceNameParameterId is null
                ? DBNull.Value
                : deviceNameParameterId);
        command.Parameters.AddWithValue(
            "$hostParameterId",
            hostParameterId);
        command.Parameters.AddWithValue(
            "$tagIdPrefixParameterId",
            tagIdPrefixParameterId);
        command.Parameters.AddWithValue(
            "$enabled",
            enabled ? 1 : 0);
        command.Parameters.AddWithValue(
            "$port",
            port);
        command.Parameters.AddWithValue(
            "$pollInterval",
            pollIntervalMilliseconds);
        command.Parameters.AddWithValue(
            "$requestTimeout",
            requestTimeoutMilliseconds);
    }
}
