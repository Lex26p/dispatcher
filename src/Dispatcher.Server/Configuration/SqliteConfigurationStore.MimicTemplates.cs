using System.Text.Json;
using Dispatcher.Server.Mimics;
using Dispatcher.Server.Templates;

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
                t.template_id,
                t.name,
                t.version,
                t.parameters_json,
                m.width,
                m.height,
                m.elements_json
            FROM templates t
            INNER JOIN mimic_templates m
                ON m.template_id = t.template_id
            WHERE t.kind = $kind
            ORDER BY t.template_id;
            """;
        command.Parameters.AddWithValue(
            "$kind",
            (int)TemplateKind.Mimic);

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
                    reader.GetString(3))
                ?? [];
            var elements =
                JsonSerializer.Deserialize<MimicTemplateElementConfiguration[]>(
                    reader.GetString(6))
                ?? [];

            var template =
                new MimicTemplateConfiguration(
                    TemplateId:
                        reader.GetString(0),
                    Name:
                        reader.GetString(1),
                    Width:
                        reader.GetInt32(4),
                    Height:
                        reader.GetInt32(5),
                    Parameters:
                        parameters,
                    Elements:
                        elements,
                    Version:
                        reader.GetInt32(2));

            MimicTemplateConfigurationValidator.Validate(
                template);

            templates.Add(
                template);
        }

        return templates;
    }

    public async Task<MimicTemplateConfiguration> UpsertMimicTemplateAsync(
        MimicTemplateConfiguration template,
        CancellationToken cancellationToken = default)
    {
        MimicTemplateConfigurationValidator.Validate(
            template);

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        using var transaction =
            connection.BeginTransaction();

        var parameters =
            template.Parameters
                .Select(parameter =>
                    new TemplateParameterConfiguration(
                        parameter.ParameterId,
                        parameter.Name))
                .ToArray();
        var version =
            await UpsertTemplateCatalogEntryAsync(
                connection,
                transaction,
                template.TemplateId,
                template.Name,
                TemplateKind.Mimic,
                parameters,
                cancellationToken);

        await using var command =
            connection.CreateCommand();

        command.Transaction =
            transaction;
        command.CommandText =
            """
            INSERT INTO mimic_templates (
                template_id,
                width,
                height,
                elements_json)
            VALUES (
                $templateId,
                $width,
                $height,
                $elementsJson)
            ON CONFLICT(template_id) DO UPDATE SET
                width = excluded.width,
                height = excluded.height,
                elements_json = excluded.elements_json;
            """;

        command.Parameters.AddWithValue(
            "$templateId",
            template.TemplateId);
        command.Parameters.AddWithValue(
            "$width",
            template.Width);
        command.Parameters.AddWithValue(
            "$height",
            template.Height);
        command.Parameters.AddWithValue(
            "$elementsJson",
            JsonSerializer.Serialize(
                template.Elements));

        await command.ExecuteNonQueryAsync(
            cancellationToken);

        transaction.Commit();

        return template with
        {
            Version = version
        };
    }

    public async Task<bool> DeleteMimicTemplateAsync(
        string templateId,
        CancellationToken cancellationToken = default)
    {
        return await DeleteTemplateAsync(
            templateId,
            TemplateKind.Mimic,
            cancellationToken);
    }
}
