using System.Net;
using System.Net.Http.Json;
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
                $"/api/mimics/{Uri.EscapeDataString(mimicId)}",
                cancellationToken);

        if (response.StatusCode == HttpStatusCode.NotFound)
        {
            return null;
        }

        response.EnsureSuccessStatusCode();

        return await response.Content.ReadFromJsonAsync<MimicDefinitionDto>(
            cancellationToken: cancellationToken);
    }
}
