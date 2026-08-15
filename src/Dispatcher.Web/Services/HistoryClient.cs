using System.Net.Http.Json;
using System.Text.Json;
using Dispatcher.Contracts.Historian;

namespace Dispatcher.Web.Services;

public sealed class HistoryClient
{
    private readonly HttpClient _httpClient;

    public HistoryClient(
        HttpClient httpClient)
    {
        _httpClient = httpClient;
    }

    public async Task<HistoryQueryResponseDto> QueryAsync(
        IReadOnlyList<string> tagIds,
        DateTimeOffset from,
        DateTimeOffset to,
        HistoryQueryOrderDto order,
        int limit,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(
            tagIds);

        var query =
            string.Join(
                "&",
                tagIds.Select(tagId =>
                    $"tagId={Uri.EscapeDataString(tagId)}"));

        var orderValue =
            order == HistoryQueryOrderDto.Ascending
                ? "asc"
                : "desc";

        var url =
            $"/api/history?{query}" +
            $"&from={Uri.EscapeDataString(from.ToUniversalTime().ToString("O"))}" +
            $"&to={Uri.EscapeDataString(to.ToUniversalTime().ToString("O"))}" +
            $"&order={orderValue}" +
            $"&limit={limit}";

        using var response =
            await _httpClient.GetAsync(
                url,
                cancellationToken);

        await EnsureSuccessAsync(
            response,
            cancellationToken);

        return await response.Content.ReadFromJsonAsync<HistoryQueryResponseDto>(
            cancellationToken: cancellationToken)
            ?? throw new InvalidOperationException(
                "Server returned an empty history response.");
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
