using System.Net.Http.Json;
using System.Text.Json;
using Dispatcher.Contracts.Alarms;

namespace Dispatcher.Web.Services;

public sealed class AlarmClient
{
    private readonly HttpClient _httpClient;

    public AlarmClient(
        HttpClient httpClient)
    {
        _httpClient = httpClient;
    }

    public async Task<IReadOnlyList<AlarmDefinitionDto>> GetDefinitionsAsync(
        CancellationToken cancellationToken = default)
    {
        return await _httpClient.GetFromJsonAsync<AlarmDefinitionDto[]>(
                "/api/configuration/alarms/definitions",
                cancellationToken)
            ?? [];
    }

    public async Task<AlarmDefinitionDto> CreateDefinitionAsync(
        CreateAlarmDefinitionRequest request,
        CancellationToken cancellationToken = default)
    {
        using var response =
            await _httpClient.PostAsJsonAsync(
                "/api/configuration/alarms/definitions",
                request,
                cancellationToken);

        return await ReadRequiredAsync<AlarmDefinitionDto>(
            response,
            cancellationToken);
    }

    public async Task<AlarmDefinitionDto> UpdateDefinitionAsync(
        string alarmId,
        UpdateAlarmDefinitionRequest request,
        CancellationToken cancellationToken = default)
    {
        using var response =
            await _httpClient.PutAsJsonAsync(
                $"/api/configuration/alarms/definitions/{Escape(alarmId)}",
                request,
                cancellationToken);

        return await ReadRequiredAsync<AlarmDefinitionDto>(
            response,
            cancellationToken);
    }

    public async Task DeleteDefinitionAsync(
        string alarmId,
        CancellationToken cancellationToken = default)
    {
        using var response =
            await _httpClient.DeleteAsync(
                $"/api/configuration/alarms/definitions/{Escape(alarmId)}",
                cancellationToken);

        await EnsureSuccessAsync(
            response,
            cancellationToken);
    }

    private static string Escape(
        string value)
    {
        return Uri.EscapeDataString(value);
    }

    private static async Task<T> ReadRequiredAsync<T>(
        HttpResponseMessage response,
        CancellationToken cancellationToken)
    {
        await EnsureSuccessAsync(
            response,
            cancellationToken);

        var result =
            await response.Content.ReadFromJsonAsync<T>(
                cancellationToken: cancellationToken);

        return result
            ?? throw new InvalidOperationException(
                "Server returned an empty alarm definition response.");
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

        if (!string.IsNullOrWhiteSpace(content))
        {
            try
            {
                using var document =
                    JsonDocument.Parse(content);
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
