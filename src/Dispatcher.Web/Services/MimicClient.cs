using System.Net;
using System.Net.Http.Json;
using System.Text.Json;
using Dispatcher.Contracts.Mimics;

namespace Dispatcher.Web.Services;

public sealed class MimicClient
{
    private readonly HttpClient _httpClient;

    public MimicClient(
        HttpClient httpClient)
    {
        _httpClient = httpClient;
    }

    public async Task<IReadOnlyList<MimicSummaryDto>> GetAllAsync(
        CancellationToken cancellationToken = default)
    {
        return await _httpClient.GetFromJsonAsync<MimicSummaryDto[]>(
            "/api/mimics",
            cancellationToken)
            ?? [];
    }

    public async Task<MimicDefinitionDto?> GetAsync(
        string mimicId,
        CancellationToken cancellationToken = default)
    {
        using var response =
            await _httpClient.GetAsync(
                $"/api/mimics/{Escape(mimicId)}",
                cancellationToken);

        if (response.StatusCode == HttpStatusCode.NotFound)
        {
            return null;
        }

        await EnsureSuccessAsync(
            response,
            cancellationToken);

        return await response.Content.ReadFromJsonAsync<MimicDefinitionDto>(
            cancellationToken: cancellationToken);
    }

    public async Task<MimicDefinitionDto> SaveAsync(
        MimicDefinitionDto definition,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(
            definition);

        using var response =
            await _httpClient.PutAsJsonAsync(
                $"/api/configuration/mimics/{Escape(definition.MimicId)}",
                definition,
                cancellationToken);

        await EnsureSuccessAsync(
            response,
            cancellationToken);

        return await response.Content.ReadFromJsonAsync<MimicDefinitionDto>(
            cancellationToken: cancellationToken)
            ?? throw new InvalidOperationException(
                "Server returned an empty mimic response.");
    }

    public async Task DeleteAsync(
        string mimicId,
        CancellationToken cancellationToken = default)
    {
        using var response =
            await _httpClient.DeleteAsync(
                $"/api/configuration/mimics/{Escape(mimicId)}",
                cancellationToken);

        await EnsureSuccessAsync(
            response,
            cancellationToken);
    }

    public async Task<IReadOnlyList<MimicTemplateDto>> GetTemplatesAsync(
        CancellationToken cancellationToken = default)
    {
        return await _httpClient.GetFromJsonAsync<MimicTemplateDto[]>(
            "/api/configuration/mimic-templates",
            cancellationToken)
            ?? [];
    }

    public async Task<MimicTemplateDto?> GetTemplateAsync(
        string templateId,
        CancellationToken cancellationToken = default)
    {
        using var response =
            await _httpClient.GetAsync(
                $"/api/configuration/mimic-templates/{Escape(templateId)}",
                cancellationToken);

        if (response.StatusCode == HttpStatusCode.NotFound)
        {
            return null;
        }

        await EnsureSuccessAsync(
            response,
            cancellationToken);

        return await response.Content.ReadFromJsonAsync<MimicTemplateDto>(
            cancellationToken: cancellationToken);
    }

    public async Task<MimicTemplateDto> SaveTemplateAsync(
        MimicTemplateDto template,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(
            template);

        using var response =
            await _httpClient.PutAsJsonAsync(
                $"/api/configuration/mimic-templates/{Escape(template.TemplateId)}",
                template,
                cancellationToken);

        await EnsureSuccessAsync(
            response,
            cancellationToken);

        return await response.Content.ReadFromJsonAsync<MimicTemplateDto>(
            cancellationToken: cancellationToken)
            ?? throw new InvalidOperationException(
                "Server returned an empty mimic template response.");
    }

    public async Task DeleteTemplateAsync(
        string templateId,
        CancellationToken cancellationToken = default)
    {
        using var response =
            await _httpClient.DeleteAsync(
                $"/api/configuration/mimic-templates/{Escape(templateId)}",
                cancellationToken);

        await EnsureSuccessAsync(
            response,
            cancellationToken);
    }

    public async Task<MimicDefinitionDto> InstantiateTemplateAsync(
        string mimicId,
        string templateId,
        InstantiateMimicTemplateRequest request,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(
            request);

        using var response =
            await _httpClient.PostAsJsonAsync(
                $"/api/configuration/mimics/{Escape(mimicId)}/templates/{Escape(templateId)}/instantiate",
                request,
                cancellationToken);

        await EnsureSuccessAsync(
            response,
            cancellationToken);

        return await response.Content.ReadFromJsonAsync<MimicDefinitionDto>(
            cancellationToken: cancellationToken)
            ?? throw new InvalidOperationException(
                "Server returned an empty mimic response after template instantiation.");
    }

    private static string Escape(
        string value)
    {
        return Uri.EscapeDataString(
            value);
    }

    private static async Task EnsureSuccessAsync(
        HttpResponseMessage response,
        CancellationToken cancellationToken)
    {
        if (response.IsSuccessStatusCode)
        {
            return;
        }

        var content =
            await response.Content.ReadAsStringAsync(
                cancellationToken);

        if (!string.IsNullOrWhiteSpace(
                content))
        {
            try
            {
                using var document =
                    JsonDocument.Parse(
                        content);

                var root =
                    document.RootElement;

                if (root.TryGetProperty(
                        "detail",
                        out var detail)
                    && detail.ValueKind == JsonValueKind.String)
                {
                    throw new InvalidOperationException(
                        detail.GetString()
                        ?? content);
                }

                if (root.TryGetProperty(
                        "title",
                        out var title)
                    && title.ValueKind == JsonValueKind.String)
                {
                    throw new InvalidOperationException(
                        title.GetString()
                        ?? content);
                }
            }
            catch (JsonException)
            {
            }

            throw new InvalidOperationException(
                content);
        }

        throw new InvalidOperationException(
            $"HTTP {(int)response.StatusCode} {response.ReasonPhrase}");
    }
}
