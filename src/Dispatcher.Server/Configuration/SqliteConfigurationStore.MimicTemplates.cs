using System.Text.Json;
using Dispatcher.Server.Mimics;

namespace Dispatcher.Server.Configuration;

public sealed partial class SqliteConfigurationStore
{
    public async Task<IReadOnlyList<MimicTemplateConfiguration>> LoadMimicTemplatesAsync(
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
                width,
                height,
                parameters_json,
                elements_json
            FROM mimic_templates
            ORDER BY template_id;
            """;

        await using var reader =
            await command.ExecuteReaderAsync(
                cancellationToken);

        var templates =
            new List<MimicTemplateConfiguration>();

        while (await reader.ReadAsync(
            cancellationToken))
        {
            var parameters =
                JsonSerializer.Deserialize<MimicTemplateParameterConfiguration[]>(
                    reader.GetString(4))
                ?? [];
            var elements =
                JsonSerializer.Deserialize<MimicTemplateElementConfiguration[]>(
                    reader.GetString(5))
                ?? [];

            var template =
                new MimicTemplateConfiguration(
                    TemplateId:
                        reader.GetString(0),
                    Name:
                        reader.GetString(1),
                    Width:
                        reader.GetInt32(2),
                    Height:
                        reader.GetInt32(3),
                    Parameters:
                        parameters,
                    Elements:
                        elements);

            MimicTemplateConfigurationValidator.Validate(
                template);

            templates.Add(
                template);
        }

        return templates;
    }

    public async Task UpsertMimicTemplateAsync(
        MimicTemplateConfiguration template,
        CancellationToken cancellationToken = default)
    {
        MimicTemplateConfigurationValidator.Validate(
            template);

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        await using var command =
            connection.CreateCommand();

        command.CommandText =
            """
            INSERT INTO mimic_templates (
                template_id,
                name,
                width,
                height,
                parameters_json,
                elements_json)
            VALUES (
                $templateId,
                $name,
                $width,
                $height,
                $parametersJson,
                $elementsJson)
            ON CONFLICT(template_id) DO UPDATE SET
                name = excluded.name,
                width = excluded.width,
                height = excluded.height,
                parameters_json = excluded.parameters_json,
                elements_json = excluded.elements_json;
            """;

        command.Parameters.AddWithValue(
            "$templateId",
            template.TemplateId);
        command.Parameters.AddWithValue(
            "$name",
            template.Name);
        command.Parameters.AddWithValue(
            "$width",
            template.Width);
        command.Parameters.AddWithValue(
            "$height",
            template.Height);
        command.Parameters.AddWithValue(
            "$parametersJson",
            JsonSerializer.Serialize(
                template.Parameters));
        command.Parameters.AddWithValue(
            "$elementsJson",
            JsonSerializer.Serialize(
                template.Elements));

        await command.ExecuteNonQueryAsync(
            cancellationToken);
    }

    public async Task<bool> DeleteMimicTemplateAsync(
        string templateId,
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
            DELETE FROM mimic_templates
            WHERE template_id = $templateId;
            """;

        command.Parameters.AddWithValue(
            "$templateId",
            templateId);

        return await command.ExecuteNonQueryAsync(
            cancellationToken) > 0;
    }
}
